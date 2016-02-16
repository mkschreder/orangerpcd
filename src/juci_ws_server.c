/*
 * Copyright (C) 2016 Martin K. Schröder <mkschreder.uk@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License version 2.1
 * as published by the Free Software Foundation
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <blobpack/blobpack.h>

#define _GNU_SOURCE
#include "juci_ws_server.h"

#include "mimetypes.h"
#include <libutype/list.h>
#include <libutype/avl.h>
#include <libwebsockets.h>
#include <pthread.h>
#include <assert.h>
#include <limits.h>

#include <libusys/usock.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/uio.h>
#include <unistd.h>
#include <fcntl.h>
#include <poll.h>

#include "juci.h"
#include "juci_id.h"
#include "internal.h"

struct lws_context; 
struct ubus_srv_ws {
	struct lws_context *ctx; 
	struct lws_protocols *protocols; 
	struct avl_tree clients; 
	//struct blob buf; 
	const struct ubus_server_api *api; 
	bool shutdown; 
	pthread_t thread; 
	pthread_mutex_t qlock; 
	pthread_cond_t rx_ready; 
	struct list_head rx_queue; 
	const char *www_root; 
	void *user_data; 
}; 

struct ubus_srv_ws_client {
	struct ubus_id id; 
	struct list_head tx_queue; 
	struct ubus_message *msg; // incoming message
	struct lws *wsi; 
	bool disconnect;
}; 

struct ubus_srv_ws_frame {
	struct list_head list; 
	uint8_t *buf; 
	int len; 
	int sent_count; 
}; 

struct ubus_srv_ws_frame *ubus_srv_ws_frame_new(struct blob_field *msg){
	assert(msg); 
	struct ubus_srv_ws_frame *self = calloc(1, sizeof(struct ubus_srv_ws_frame)); 
	INIT_LIST_HEAD(&self->list); 
	char *json = blob_field_to_json(msg); 
	self->len = strlen(json); 
	self->buf = calloc(1, LWS_SEND_BUFFER_PRE_PADDING + self->len + LWS_SEND_BUFFER_POST_PADDING); 
	memcpy(self->buf + LWS_SEND_BUFFER_PRE_PADDING, json, self->len); 
	free(json); 
	return self; 
}

void ubus_srv_ws_frame_delete(struct ubus_srv_ws_frame **self){
	assert(self && *self); 
	free((*self)->buf); 
	free(*self); 
	*self = NULL; 
}


static struct ubus_srv_ws_client *ubus_srv_ws_client_new(){
	struct ubus_srv_ws_client *self = calloc(1, sizeof(struct ubus_srv_ws_client)); 
	INIT_LIST_HEAD(&self->tx_queue); 
	self->msg = ubus_message_new(); 
	return self; 
}

static __attribute__((unused)) void ubus_srv_ws_client_delete(struct ubus_srv_ws_client **self){
	// TODO: free tx_queue
	struct ubus_srv_ws_frame *pos, *tmp; 
	list_for_each_entry_safe(pos, tmp, &(*self)->tx_queue, list){
		ubus_srv_ws_frame_delete(&pos);  
	}	
	ubus_message_delete(&(*self)->msg); 
	free(*self); 
	*self = NULL;
}

static int _ubus_socket_callback(struct lws *wsi, enum lws_callback_reasons reason, void *_user, void *in, size_t len){
	// TODO: keeping user data in protocol is probably not the right place. Fix it. 
	const struct lws_protocols *proto = lws_get_protocol(wsi); 

	struct ubus_srv_ws_client **user = (struct ubus_srv_ws_client **)_user; 
	
	if(user && *user && (*user)->disconnect) return -1; 
	
	//printf("%d.", reason); fflush(stdout); 

	int32_t peer_id = lws_get_socket_fd(wsi); 
	switch(reason){
		case LWS_CALLBACK_ESTABLISHED: {
			struct ubus_srv_ws *self = (struct ubus_srv_ws*)proto->user; 
			struct ubus_srv_ws_client *client = ubus_srv_ws_client_new(lws_get_socket_fd(wsi)); 
			pthread_mutex_lock(&self->qlock); 
			ubus_id_alloc(&self->clients, &client->id, 0); 
			*user = client; 
			char hostname[255], ipaddr[255]; 
			lws_get_peer_addresses(wsi, peer_id, hostname, sizeof(hostname), ipaddr, sizeof(ipaddr)); 
			DEBUG("connection established! %s %s %d %08x\n", hostname, ipaddr, peer_id, client->id.id); 
			//if(self->on_message) self->on_message(&self->api, (*user)->id.id, UBUS_MSG_PEER_CONNECTED, 0, NULL); 
			client->wsi = wsi; 
			pthread_mutex_unlock(&self->qlock); 
			lws_callback_on_writable(wsi); 	
			break; 
		}
		case LWS_CALLBACK_CLIENT_CONNECTION_ERROR:
			DEBUG("websocket: client error\n"); 
			break; 
		case LWS_CALLBACK_CLOSED: {
			DEBUG("websocket: client disconnected %p %p\n", _user, *user); 
			struct ubus_srv_ws *self = (struct ubus_srv_ws*)proto->user; 
			pthread_mutex_lock(&self->qlock); 
			//if(self->on_message) self->on_message(&self->api, (*user)->id.id, UBUS_MSG_PEER_DISCONNECTED, 0, NULL); 
			ubus_id_free(&self->clients, &(*user)->id); 
			ubus_srv_ws_client_delete(user); 	
			pthread_mutex_unlock(&self->qlock); 
			*user = 0; 
			break; 
		}
		case LWS_CALLBACK_SERVER_WRITEABLE: {
			struct ubus_srv_ws *self = (struct ubus_srv_ws*)proto->user; 
			pthread_mutex_lock(&self->qlock); 
			while(!list_empty(&(*user)->tx_queue)){
				// TODO: handle partial writes correctly 
				struct ubus_srv_ws_frame *frame = list_first_entry(&(*user)->tx_queue, struct ubus_srv_ws_frame, list);
				int left = frame->len - frame->sent_count; 
				int len = left; 
				int flags; 
				if(frame->sent_count == 0){
					flags = LWS_WRITE_TEXT; 
				} else {
					flags = LWS_WRITE_CONTINUATION; 
				}
				// fragment the message by standard mtu size. 
				if(left > 1500){
					len = 1500; 
					flags |= LWS_WRITE_NO_FIN; 
				} 
				int n = lws_write(wsi, &frame->buf[LWS_SEND_BUFFER_PRE_PADDING]+frame->sent_count, len, flags);
				if(n < 0) { 
					DEBUG("error while sending data over websocket!\n"); 
					pthread_mutex_unlock(&self->qlock); 
					// disconnect
					return 1; 
				}
				frame->sent_count += n; 
				DEBUG("sent %d out of %d bytes\n", frame->sent_count, frame->len); 
				if(frame->sent_count >= frame->len){
					list_del_init(&frame->list); 
					ubus_srv_ws_frame_delete(&frame); 
				} else {
					pthread_mutex_unlock(&self->qlock); 
					printf("not final fragment!\n"); 
					lws_callback_on_writable(wsi); 
					return 0; 
				}
			}
			pthread_mutex_unlock(&self->qlock); 
			lws_rx_flow_control(wsi, 1); 
			break; 
		}
		case LWS_CALLBACK_RECEIVE: {
			assert(proto); 
			assert(user); 
			if(!user) break; 
			struct ubus_srv_ws *self = (struct ubus_srv_ws*)proto->user; 
			pthread_mutex_lock(&self->qlock); 
			blob_reset(&(*user)->msg->buf); 
			if(blob_put_json(&(*user)->msg->buf, in)){
				//struct blob_field *rpcobj = blob_field_first_child(blob_head(&self->buf)); 
				//TODO: add message to queue
				//blob_dump_json(&(*user)->msg->buf); 

				// place the message on the queue
				(*user)->msg->peer = (*user)->id.id; 
				list_add_tail(&(*user)->msg->list, &self->rx_queue); 
				(*user)->msg = ubus_message_new(); 
				pthread_cond_signal(&self->rx_ready); 
			} else {
				ERROR("got bad message\n"); 
			}
			pthread_mutex_unlock(&self->qlock); 
			lws_rx_flow_control(wsi, 0); 
			lws_callback_on_writable(wsi); 	
			break; 
		}
		/*case LWS_CALLBACK_HTTP: {
			struct ubus_srv_ws *self = (struct ubus_srv_ws*)proto->user; 
            char *requested_uri = (char *) in;
            DEBUG("requested URI: %s\n", requested_uri);
           
            if (strcmp(requested_uri, "/") == 0) 
				requested_uri = "/index.html"; 

			// allocate enough memory for the resource path
			char *resource_path = malloc(strlen(self->www_root) + strlen(requested_uri) + 1);
		   
			// join current working direcotry to the resource path
			sprintf(resource_path, "%s%s", self->www_root, requested_uri);
			DEBUG("resource path: %s\n", resource_path);
		   
			char *extension = strrchr(resource_path, '.');
		  	const char *mime = mimetype_lookup(extension); 
		   
			// by default non existing resources return code 404
			lws_serve_http_file(wsi, resource_path, mime, NULL, 0);
			// we have to check this otherwise we will get incomplete transfers
			if(lws_send_pipe_choked(wsi)) break; 
			// return 1 so that the connection shall be closed
			return 1; 
       	} break;  */   
		default: { 
			
		} break; 
	}; 
	return 0; 
}

void _websocket_destroy(juci_server_t socket){
	struct ubus_srv_ws *self = container_of(socket, struct ubus_srv_ws, api); 
	self->shutdown = true; 
	DEBUG("websocket: joining worker thread..\n"); 
	pthread_join(self->thread, NULL); 
	pthread_mutex_destroy(&self->qlock); 
	pthread_cond_destroy(&self->rx_ready); 
	struct ubus_id *id, *tmp; 
	avl_for_each_element_safe(&self->clients, id, avl, tmp){
		struct ubus_srv_ws_client *client = container_of(id, struct ubus_srv_ws_client, id);  
		ubus_id_free(&self->clients, &client->id); 
		ubus_srv_ws_client_delete(&client); 
	}

	if(self->ctx) lws_context_destroy(self->ctx); 
	DEBUG("websocket: context destroyed\n"); 
	free(self->protocols); 
	free(self);  
}

int _websocket_listen(juci_server_t socket, const char *path){
	struct ubus_srv_ws *self = container_of(socket, struct ubus_srv_ws, api); 
	struct lws_context_creation_info info; 
	memset(&info, 0, sizeof(info)); 

	char proto[NAME_MAX], host[NAME_MAX], file[NAME_MAX]; 
	int port = 5303; 
	if(!url_scanf(path, proto, host, &port, file)){
		fprintf(stderr, "Could not parse url: %s\n", path); 
		return -1; 
	}

	info.port = port;
	info.gid = -1; 
	info.uid = -1; 
	info.user = self; 
	info.protocols = self->protocols; 
	//info.extensions = lws_get_internal_extensions();
	info.options = LWS_SERVER_OPTION_VALIDATE_UTF8;

	self->ctx = lws_create_context(&info); 

	return 0; 
}

int _websocket_connect(juci_server_t socket, const char *path){
	//struct ubus_srv_ws *self = container_of(socket, struct ubus_srv_ws, api); 
	return -1; 
}

static void *_websocket_server_thread(void *ptr){
	struct ubus_srv_ws *self = (struct ubus_srv_ws*)ptr; 
	while(!self->shutdown){
		if(!self->ctx) { sleep(1); continue;} 
		lws_service(self->ctx, 10);	
	}
	pthread_exit(0); 
	return 0; 
}

static int _websocket_send(juci_server_t socket, struct ubus_message **msg){
	struct ubus_srv_ws *self = container_of(socket, struct ubus_srv_ws, api); 
	pthread_mutex_lock(&self->qlock); 
	struct ubus_id *id = ubus_id_find(&self->clients, (*msg)->peer); 
	if(!id) {
		pthread_mutex_unlock(&self->qlock); 
		return -1; 
	}
	
	struct ubus_srv_ws_client *client = (struct ubus_srv_ws_client*)container_of(id, struct ubus_srv_ws_client, id);  
	struct ubus_srv_ws_frame *frame = ubus_srv_ws_frame_new(blob_head(&(*msg)->buf)); 
	list_add_tail(&frame->list, &client->tx_queue); 	
	pthread_mutex_unlock(&self->qlock); 
	lws_callback_on_writable(client->wsi); 	
	ubus_message_delete(msg); 
	return 0; 
}

static void *_websocket_userdata(juci_server_t socket, void *ptr){
	struct ubus_srv_ws *self = container_of(socket, struct ubus_srv_ws, api); 
	if(!ptr) return self->user_data; 
	self->user_data = ptr; 
	return ptr; 
}

static void _timeout_us(struct timespec *t, unsigned long long timeout_us){
	unsigned long long sec = (timeout_us >= 1000000UL)?(timeout_us / 1000000UL):0; 
	unsigned long long nsec = (timeout_us - sec * 1000000UL) * 1000UL; 

	// figure out the exact timeout we should wait for the conditional 
	clock_gettime(CLOCK_REALTIME, t); 
	t->tv_nsec += nsec; 
	t->tv_sec += sec; 
	// nanoseconds can sometimes be more than 1sec but never more than 2.  
	// TODO: is there no function somewhere for properly adding a timeout to timespec? 
	if(t->tv_nsec >= 1000000000UL){
		t->tv_nsec -= 1000000000UL; 
		t->tv_sec++; 
	}
}

static int _websocket_recv(juci_server_t socket, struct ubus_message **msg, unsigned long long timeout_us){
	struct ubus_srv_ws *self = container_of(socket, struct ubus_srv_ws, api); 

	struct timespec t; 
	_timeout_us(&t, timeout_us); 

	// lock mutex to check for the queue
	pthread_mutex_lock(&self->qlock); 
	if(list_empty(&self->rx_queue)){
		// unlock the mutex and wait for a new event. timeout if no event received for a timeout.  
		// this will lock mutex after either timeout or if condition is signalled
		if(pthread_cond_timedwait(&self->rx_ready, &self->qlock, &t) == ETIMEDOUT){
			// we still need to unlock the mutex before we exit
			pthread_mutex_unlock(&self->qlock); 
			return -EAGAIN; 
		}
	}

	// pop a message from the stack
	struct ubus_message *m = list_first_entry(&self->rx_queue, struct ubus_message, list); 
	list_del_init(&m->list); 
	*msg = m; 

	// unlock mutex and return with positive number to signal message received. 
	pthread_mutex_unlock(&self->qlock); 
	return 1; 
}

juci_server_t juci_ws_server_new(const char *www_root){
	struct ubus_srv_ws *self = calloc(1, sizeof(struct ubus_srv_ws)); 
	self->www_root = (www_root)?www_root:"/www/"; 
	self->protocols = calloc(2, sizeof(struct lws_protocols)); 
	self->protocols[0] = (struct lws_protocols){
		.name = "",
		.callback = _ubus_socket_callback,
		.per_session_data_size = sizeof(struct ubus_srv_ws_client*),
		.user = self
	};
	ubus_id_tree_init(&self->clients); 
	pthread_mutex_init(&self->qlock, NULL); 
	pthread_cond_init(&self->rx_ready, NULL); 
	INIT_LIST_HEAD(&self->rx_queue); 
	static const struct ubus_server_api api = {
		.destroy = _websocket_destroy, 
		.listen = _websocket_listen, 
		.connect = _websocket_connect, 
		.send = _websocket_send, 
		.recv = _websocket_recv, 
		.userdata = _websocket_userdata
	}; 
	self->api = &api; 
	pthread_create(&self->thread, NULL, _websocket_server_thread, self); 
	return &self->api; 
}
