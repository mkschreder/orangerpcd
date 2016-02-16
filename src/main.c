#define _XOPEN_SOURCE
#define _XOPEN_SOURCE_EXTENDED
#define _BSD_SOURCE
#define _POSIX_C_SOURCE 199309L

#include <time.h>
#include <stdio.h>
#include <unistd.h>
#include <dirent.h>
#include <signal.h>

#include <libutype/avl-cmp.h>

#include "juci.h"
#include "juci_luaobject.h"
#include "juci_ws_server.h"

bool running = true; 

void handle_sigint(){
	DEBUG("Interrupted!\n"); 
	running = false; 
}

static bool rpcmsg_parse_call(struct blob *msg, uint32_t *id, struct blob_field **method, struct blob_field **params){
	if(!msg) return false; 
	struct blob_policy policy[] = {
		{ .name = "id", .type = BLOB_FIELD_ANY, .value = NULL },
		{ .name = "method", .type = BLOB_FIELD_STRING, .value = NULL },
		{ .name = "params", .type = BLOB_FIELD_ARRAY, .value = NULL }
	}; 
	struct blob_field *b = blob_field_first_child(blob_head(msg));  
	blob_field_parse_values(b, policy, 3); 
	*id = blob_field_get_int(policy[0].value); 
	*method = policy[1].value; 
	*params = policy[2].value; 
	return !!(policy[0].value && method && params); 
}

static bool rpcmsg_parse_params(struct blob_field *params, struct blob_field **object, struct blob_field **method, struct blob_field **args){
	if(!params) return false; 
	struct blob_policy policy[] = {
		{ .type = BLOB_FIELD_STRING },
		{ .type = BLOB_FIELD_STRING },
		{ .type = BLOB_FIELD_TABLE }
	}; 
	blob_field_parse_values(params, policy, 3); 
	*object = policy[0].value; 
	*method = policy[1].value; 
	*args = policy[2].value; 
	return !!(object && method && args); 

}

int main(int argc, char **argv){
  	const char *www_root = "/www"; 
	const char *listen_socket = "ws://localhost:1234"; 
	const char *plugin_dir = "plugins"; 

	int c = 0; 	
	while((c = getopt(argc, argv, "d:l:p:v")) != -1){
		switch(c){
			case 'd': 
				www_root = optarg; 
				break; 
			case 'l':
				listen_socket = optarg; 
				break; 
			case 'p': 
				plugin_dir = optarg; 
				break; 
			case 'v': 
				juci_debug_level++; 
				break; 
			default: break; 
		}
	}

    juci_server_t server = juci_ws_server_new(www_root); 

    if(ubus_server_listen(server, listen_socket) < 0){
        fprintf(stderr, "server could not listen on specified socket!\n"); 
        return -1;                       
    }

	signal(SIGINT, handle_sigint); 

	struct juci app; 
	juci_init(&app); 

	juci_load_plugins(&app, plugin_dir, NULL); 
	
	struct blob buf, out; 
	blob_init(&buf, 0, 0); 
	blob_init(&out, 0, 0); 

    while(running){                     
        struct ubus_message *msg = NULL;         
		struct timespec tss, tse; 
		clock_gettime(CLOCK_MONOTONIC, &tss); 
		
		// 10ms delay 
        if(ubus_server_recv(server, &msg, 10000UL) < 0){  
            continue;                   
        }
		clock_gettime(CLOCK_MONOTONIC, &tse); 
		TRACE("waited %lus %luns for message\n", tse.tv_sec - tss.tv_sec, tse.tv_nsec - tss.tv_nsec); 
        if(juci_debug_level > 2){
			DEBUG("got message from %08x: ", msg->peer); 
        	blob_dump_json(&msg->buf);
		}
		struct blob_field *rpc_method = NULL, *params = NULL, *method = NULL, *object = NULL, *args = NULL; 
		uint32_t rpc_id = 0; 
		rpcmsg_parse_call(&msg->buf, &rpc_id, &rpc_method, &params); 

		struct ubus_message *result = ubus_message_new(); 
		result->peer = msg->peer; 

		blob_offset_t t = blob_open_table(&result->buf); 
		blob_put_string(&result->buf, "jsonrpc"); 
		blob_put_string(&result->buf, "2.0"); 
		blob_put_string(&result->buf, "id"); blob_put_int(&result->buf, rpc_id); 
		blob_put_string(&result->buf, "result"); 

		if(rpc_method && strcmp(blob_field_get_string(rpc_method), "call") == 0){
			if(!rpcmsg_parse_params(params, &object, &method, &args)){
				DEBUG("Could not parse call params!\n"); 
			}
			juci_call(&app, blob_field_get_string(object), blob_field_get_string(method), args, &result->buf); 
		} else if(rpc_method && strcmp(blob_field_get_string(rpc_method), "list") == 0){
			rpcmsg_parse_params(params, &object, &method, &args); 
			juci_list(&app, blob_field_get_string(object), &result->buf); 
		}

		blob_close_table(&result->buf, t); 
		if(juci_debug_level > 2){
			DEBUG("sending back: "); 
			blob_dump_json(&result->buf); 
		}
		ubus_server_send(server, &result); 		
    }

	DEBUG("cleaning up\n"); 
	ubus_server_delete(server); 

	return 0; 
}
