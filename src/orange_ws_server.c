/*
	JUCI Backend Websocket API Server

	Copyright (C) 2016 Martin K. Schröder <mkschreder.uk@gmail.com>

	This program is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version. (Please read LICENSE file on special
	permission to include this software in signed images). 

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.
*/

#include <blobpack/blobpack.h>

#define _GNU_SOURCE
#include "orange_ws_server.h"

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

#include "orange.h"
#include "orange_id.h"
#include "internal.h"
#include "json_check.h"

struct lws_context; 
struct orange_srv_ws {
	struct lws_context *ctx; 
	struct lws_protocols *protocols; 
	struct avl_tree clients; 
	//struct blob buf; 
	const struct orange_server_api *api; 
	bool shutdown; 
	pthread_t thread; 
	pthread_mutex_t lock; 
	pthread_mutex_t qlock; 
	pthread_cond_t rx_ready; 
	struct list_head rx_queue; 
	const char *www_root; 
	void *user_data; 
	JSON_check jc; 
}; 

struct orange_srv_ws_client {
	struct orange_id id; 
	struct list_head tx_queue; 
	struct orange_message *msg; // incoming message
	struct lws *wsi; 

	char buffer[32768]; 
	int buffer_start; 

	bool disconnect;
}; 

struct orange_srv_ws_frame {
	struct list_head list; 
	uint8_t *buf; 
	int len; 
	int sent_count; 
}; 

static bool url_scanf(const char *url, char *proto, char *host, int *port, char *page){
    if (sscanf(url, "%99[^:]://%99[^:]:%i/%199[^\n]", proto, host, port, page) == 4) return true; 
    else if (sscanf(url, "%99[^:]://%99[^/]/%199[^\n]", proto, host, page) == 3) return true; 
    else if (sscanf(url, "%99[^:]://%99[^:]:%i[^\n]", proto, host, port) == 3) return true; 
    else if (sscanf(url, "%99[^:]://%99[^\n]", proto, host) == 2) return true; 
    return false;                       
}

static struct orange_srv_ws_frame *orange_srv_ws_frame_new(struct blob_field *msg){
	assert(msg); 
	struct orange_srv_ws_frame *self = calloc(1, sizeof(struct orange_srv_ws_frame)); 
	assert(self); 
	INIT_LIST_HEAD(&self->list); 
	char *json = blob_field_to_json(msg); 
	self->len = strlen(json); 
	self->buf = calloc(1, LWS_SEND_BUFFER_PRE_PADDING + self->len + LWS_SEND_BUFFER_POST_PADDING); 
	assert(self->buf); 
	memcpy(self->buf + LWS_SEND_BUFFER_PRE_PADDING, json, self->len); 
	free(json); 
	return self; 
}

static void orange_srv_ws_frame_delete(struct orange_srv_ws_frame **self){
	assert(self && *self); 
	free((*self)->buf); 
	free(*self); 
	*self = NULL; 
}


static struct orange_srv_ws_client *orange_srv_ws_client_new(void){
	struct orange_srv_ws_client *self = calloc(1, sizeof(struct orange_srv_ws_client)); 
	assert(self); 
	INIT_LIST_HEAD(&self->tx_queue); 
	self->msg = orange_message_new(); 
	return self; 
}

static void orange_srv_ws_client_delete(struct orange_srv_ws_client **self){
	// TODO: free tx_queue
	struct orange_srv_ws_frame *pos, *tmp; 
	list_for_each_entry_safe(pos, tmp, &(*self)->tx_queue, list){
		orange_srv_ws_frame_delete(&pos);  
	}	
	orange_message_delete(&(*self)->msg); 
	free(*self); 
	*self = NULL;
}

static int _orange_socket_callback(struct lws *wsi, enum lws_callback_reasons reason, void *_user, void *in, size_t len){
	// TODO: keeping user data in protocol is probably not the right place. Fix it. 
	const struct lws_protocols *proto = lws_get_protocol(wsi); 

	struct orange_srv_ws_client **user = (struct orange_srv_ws_client **)_user; 
	
	if(user && *user && (*user)->disconnect){
		DEBUG("ws_client requested a disconnect!\n"); 
		return -1; 
	}
	
	int32_t peer_id = lws_get_socket_fd(wsi); 
	switch(reason){
		case LWS_CALLBACK_ESTABLISHED: {
			struct orange_srv_ws *self = (struct orange_srv_ws*)proto->user; 
			pthread_mutex_lock(&self->qlock); 
			struct orange_srv_ws_client *client = orange_srv_ws_client_new(); 
			orange_id_alloc(&self->clients, &client->id, 0); 
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
			struct orange_srv_ws *self = (struct orange_srv_ws*)proto->user; 
			pthread_mutex_lock(&self->qlock); 
			//if(self->on_message) self->on_message(&self->api, (*user)->id.id, UBUS_MSG_PEER_DISCONNECTED, 0, NULL); 
			orange_id_free(&self->clients, &(*user)->id); 
			orange_srv_ws_client_delete(user); 	
			pthread_mutex_unlock(&self->qlock); 
			*user = 0; 
			break; 
		}
		case LWS_CALLBACK_SERVER_WRITEABLE: {
			struct orange_srv_ws *self = (struct orange_srv_ws*)proto->user; 
			pthread_mutex_lock(&self->qlock); 
			while(!list_empty(&(*user)->tx_queue)){
				// TODO: handle partial writes correctly 
				struct orange_srv_ws_frame *frame = list_first_entry(&(*user)->tx_queue, struct orange_srv_ws_frame, list);
				do {
					int left = frame->len - frame->sent_count; 
					int towrite = left; 
					int flags; 
					if(frame->sent_count == 0){
						flags = LWS_WRITE_TEXT; 
					} else {
						flags = LWS_WRITE_CONTINUATION; 
					}

					// fragment the message by standard mtu size. 
					if(left > 1500){
						towrite = 1500; 
						flags |= LWS_WRITE_NO_FIN; 
					} 

					int n = lws_write(wsi, &frame->buf[LWS_SEND_BUFFER_PRE_PADDING]+frame->sent_count, towrite, flags);
					if(n < 0) { 
						DEBUG("error while sending data over websocket!\n"); 
						pthread_mutex_unlock(&self->qlock); 
						// disconnect
						return 1; 
					}
					// increment sent count
					frame->sent_count += n; 

					DEBUG("sent %d out of %d bytes\n", frame->sent_count, frame->len); 

				} while(frame->sent_count < frame->len && !lws_partial_buffered(wsi));  
	
				// FIXME: is this always going to be called at the right time? 
				if(frame->sent_count >= frame->len){
					list_del_init(&frame->list); 
					orange_srv_ws_frame_delete(&frame); 
				} 

				// if there is more then we need to tell lws to call us again
				if(lws_partial_buffered(wsi)){
					lws_callback_on_writable(wsi); 
					break; 
				} 
			}
			pthread_mutex_unlock(&self->qlock); 
			// FIXME: we need to only call this when we actually either have more data to write. But we do this every time for now just to make sure server works. 
			lws_callback_on_writable(wsi); 	
			//lws_rx_flow_control(wsi, 1); 
			break; 
		}
		case LWS_CALLBACK_RECEIVE: {
			assert(proto); 
			assert(user); 
			if(!user) break; 
			struct orange_srv_ws *self = (struct orange_srv_ws*)proto->user; 
			if((*user)->buffer_start + len > sizeof((*user)->buffer)){
				// messages larger than maximum size are discarded
				(*user)->buffer_start = 0; 
				ERROR("message too large! Discarded!\n"); 
				break; 
			}
			TRACE("received fragment of %d bytes\n", (int)len); 
			if(lws_is_final_fragment(wsi)){
				// write final fragment
				char *ptr = (*user)->buffer + (*user)->buffer_start; 
				memcpy(ptr, in, len); 
				ptr[len] = 0; 

				blob_reset(&(*user)->msg->buf); 

				// if message is small and we have received all of it then skip the scratch buffer and process it directly 
				if(!JSON_check_string(self->jc, (*user)->buffer) || !blob_put_json(&(*user)->msg->buf, (*user)->buffer)){
					ERROR("got bad message: %s\n", (*user)->buffer); 
					break; 
				}
				// place the message on the queue
				(*user)->msg->peer = (*user)->id.id; 
				pthread_mutex_lock(&self->qlock); 
				list_add_tail(&(*user)->msg->list, &self->rx_queue); 
				(*user)->msg = orange_message_new(); 
				blob_reset(&(*user)->msg->buf); 
				(*user)->buffer_start = 0; 
				pthread_cond_signal(&self->rx_ready); 
				pthread_mutex_unlock(&self->qlock); 
			} else {
				// write to scratch buffer
				char *ptr = (*user)->buffer + (*user)->buffer_start; 
				memcpy(ptr, in, len); 
				ptr[len] = 0; 
				(*user)->buffer_start += len; 
			}
			//lws_rx_flow_control(wsi, 0); 
			//lws_callback_on_writable(wsi); 	
			break; 
		}
		/*case LWS_CALLBACK_HTTP: {
			struct orange_srv_ws *self = (struct orange_srv_ws*)proto->user; 
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

static void _websocket_destroy(orange_server_t socket){
	struct orange_srv_ws *self = container_of(socket, struct orange_srv_ws, api); 

	pthread_mutex_lock(&self->lock); 
	self->shutdown = true; 
	pthread_mutex_unlock(&self->lock); 

	DEBUG("websocket: joining worker thread..\n"); 
	pthread_join(self->thread, NULL); 

	if(self->ctx) lws_context_destroy(self->ctx); 

	pthread_mutex_destroy(&self->qlock); 
	pthread_cond_destroy(&self->rx_ready); 

	struct orange_id *id, *tmp; 
	avl_for_each_element_safe(&self->clients, id, avl, tmp){
		struct orange_srv_ws_client *client = container_of(id, struct orange_srv_ws_client, id);  
		orange_id_free(&self->clients, &client->id); 
		orange_srv_ws_client_delete(&client); 
	}
	
	struct orange_message *msg, *nmsg; 
	list_for_each_entry_safe(msg, nmsg, &self->rx_queue, list){
		orange_message_delete(&msg); 
	}

	JSON_check_free(&self->jc); 

	DEBUG("websocket: context destroyed\n"); 
	free(self->protocols); 
	free(self);  
}

#include <arpa/inet.h>
#include <net/if.h>
#include <ifaddrs.h>

static int _find_interface_from_ip(const char *ip, char *ifname, size_t out_size){
	struct ifaddrs *addrs, *iap;
	struct sockaddr_in *sa;
	getifaddrs(&addrs);
	for (iap = addrs; iap != NULL; iap = iap->ifa_next) {
		if(!iap->ifa_addr) continue; // some interfaces can have this set to null! 
		int family = iap->ifa_addr->sa_family; 
		if ((iap->ifa_flags & IFF_UP) && (family == AF_INET || family == AF_INET6)) {
			char host[NI_MAXHOST+1]; 	
			int s = getnameinfo(iap->ifa_addr,
				   (family == AF_INET) ? sizeof(struct sockaddr_in) :
										 sizeof(struct sockaddr_in6),
				   host, NI_MAXHOST,
				   NULL, 0, NI_NUMERICHOST);
			if (s != 0) {
			   printf("getnameinfo() failed: %s\n", gai_strerror(s));
			} else if(strcmp(ip, host) == 0){
				strncpy(ifname, iap->ifa_name, out_size); 
				freeifaddrs(addrs); 
				return 0; 
			}
		}
	}
	freeifaddrs(addrs);
	return -ENOENT; 
}

static int _websocket_listen(orange_server_t socket, const char *path){
	struct orange_srv_ws *self = container_of(socket, struct orange_srv_ws, api); 
	struct lws_context_creation_info info; 
	memset(&info, 0, sizeof(info)); 

	char proto[NAME_MAX], ip[NAME_MAX], file[NAME_MAX], ifname[16]; 
	int port = 5303; 
	if(!url_scanf(path, proto, ip, &port, file)){
		fprintf(stderr, "Could not parse url: %s\n", path); 
		return -1; 
	}

	if(strlen(ip) && _find_interface_from_ip(ip, ifname, sizeof(ifname)) < 0){
		fprintf(stderr, "Could not find interface on which to listen for '%s' when creating websocket!\n", ip); 
		return -1; 
	}

	if(strlen(ifname)){
		info.iface = ifname; 
		INFO("WARNING: no interface name supplied so will be listening on ALL interfaces. If this is not what you want then supply an ip address on which to listen!\n"); 
	}	
	
	DEBUG("starting server on '%s:%d'\n", (strlen(ifname))?ifname:"*", port); 

	info.port = port;
	info.gid = -1; 
	info.uid = -1; 
	info.user = self; 
	info.protocols = self->protocols; 
	//info.extensions = lws_get_internal_extensions();
	info.options = LWS_SERVER_OPTION_VALIDATE_UTF8;

	pthread_mutex_lock(&self->lock); 
	self->ctx = lws_create_context(&info); 
	pthread_mutex_unlock(&self->lock); 

	return 0; 
}

static int _websocket_connect(orange_server_t socket, const char *path){
	//struct orange_srv_ws *self = container_of(socket, struct orange_srv_ws, api); 
	return -1; 
}

static void *_websocket_server_thread(void *ptr){
	struct orange_srv_ws *self = (struct orange_srv_ws*)ptr; 
	pthread_mutex_lock(&self->lock); 
	while(!self->shutdown){
		if(self->ctx){
			pthread_mutex_unlock(&self->lock); 
			lws_service(self->ctx, 10);	
			pthread_mutex_lock(&self->lock); 
		} else {
			pthread_mutex_unlock(&self->lock); 
			usleep(1000); 
			pthread_mutex_lock(&self->lock); 
		}
	}
	pthread_mutex_unlock(&self->lock); 
	pthread_exit(0); 
	return 0; 
}

static int _websocket_send(orange_server_t socket, struct orange_message **msg){
	struct orange_srv_ws *self = container_of(socket, struct orange_srv_ws, api); 
	pthread_mutex_lock(&self->qlock); 
	struct orange_id *id = orange_id_find(&self->clients, (*msg)->peer); 
	if(!id) {
		pthread_mutex_unlock(&self->qlock); 
		return -1; 
	}
	
	struct orange_srv_ws_client *client = (struct orange_srv_ws_client*)container_of(id, struct orange_srv_ws_client, id);  
	struct orange_srv_ws_frame *frame = orange_srv_ws_frame_new(blob_field_first_child(blob_head(&(*msg)->buf))); 
	list_add_tail(&frame->list, &client->tx_queue); 	
	pthread_mutex_unlock(&self->qlock); 
	orange_message_delete(msg); 
	return 0; 
}

static void *_websocket_userdata(orange_server_t socket, void *ptr){
	struct orange_srv_ws *self = container_of(socket, struct orange_srv_ws, api); 
	pthread_mutex_lock(&self->lock); 
	if(!ptr) {
		void *ret = self->user_data; 
		pthread_mutex_unlock(&self->lock); 
		return ret; 
	}
	self->user_data = ptr; 
	pthread_mutex_unlock(&self->lock); 
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
	if(t->tv_nsec >= 1000000000L){
		t->tv_nsec -= 1000000000L; 
		t->tv_sec++; 
	}
}

static int _websocket_recv(orange_server_t socket, struct orange_message **msg, unsigned long long timeout_us){
	struct orange_srv_ws *self = container_of(socket, struct orange_srv_ws, api); 

	*msg = NULL; 

	pthread_mutex_lock(&self->lock); 
	if(self->shutdown) {
		pthread_mutex_unlock(&self->lock); 
		return -1; 
	}
	pthread_mutex_unlock(&self->lock); 

	pthread_mutex_lock(&self->qlock); 
	struct timespec t; 
	_timeout_us(&t, timeout_us); 

	// lock mutex to check for the queue
	while(list_empty(&self->rx_queue)){
		// unlock the mutex and wait for a new event. timeout if no event received for a timeout.  
		// this will lock mutex after either timeout or if condition is signalled
		if(pthread_cond_timedwait(&self->rx_ready, &self->qlock, &t) == ETIMEDOUT){
			// we still need to unlock the mutex before we exit
			pthread_mutex_unlock(&self->qlock); 
			return -EAGAIN; 
		}
	}

	if(list_empty(&self->rx_queue)) printf("LIST IS EMPTY!!\n"); 
	// pop a message from the stack
	struct orange_message *m = list_first_entry(&self->rx_queue, struct orange_message, list); 
	list_del_init(&m->list); 
	*msg = m; 

	// unlock mutex and return with positive number to signal message received. 
	pthread_mutex_unlock(&self->qlock); 
	return 1; 
}

orange_server_t orange_ws_server_new(const char *www_root){
	struct orange_srv_ws *self = calloc(1, sizeof(struct orange_srv_ws)); 
	assert(self); 
	self->www_root = (www_root)?www_root:"/www/"; 
	self->protocols = calloc(2, sizeof(struct lws_protocols)); 
	assert(self->protocols); 
	self->protocols[0] = (struct lws_protocols){
		.name = "rpc",
		.callback = _orange_socket_callback,
		.per_session_data_size = sizeof(struct orange_srv_ws_client*),
		.user = self
	};
	orange_id_tree_init(&self->clients); 
	pthread_mutex_init(&self->lock, NULL); 
	pthread_mutex_init(&self->qlock, NULL); 
	pthread_cond_init(&self->rx_ready, NULL); 
	INIT_LIST_HEAD(&self->rx_queue); 
	static const struct orange_server_api api = {
		.destroy = _websocket_destroy, 
		.listen = _websocket_listen, 
		.connect = _websocket_connect, 
		.send = _websocket_send, 
		.recv = _websocket_recv, 
		.userdata = _websocket_userdata
	}; 
	self->api = &api; 
	self->jc = JSON_check_new(10); 
	pthread_create(&self->thread, NULL, _websocket_server_thread, self); 
	return &self->api; 
}
