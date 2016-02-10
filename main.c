#define _XOPEN_SOURCE
#define _XOPEN_SOURCE_EXTENDED
#define _BSD_SOURCE

#include <stdio.h>
#include <unistd.h>
#include <dirent.h>

#include <libubus2/libubus2.h>
#include <libubus2/ubus_srv_ws.h>
#include <libubus2/ubus_cli_js.h>
#include <libutype/avl-cmp.h>

#include "juci_luaobject.h"

bool running = true; 

void handle_sigint(){
	printf("Interrupted!\n"); 
	running = false; 
}

struct app {
	struct avl_tree objects; 
	lua_State *lua; 
}; 

static int load_plugins(struct app *self, const char *path, const char *base_path){
    int rv = 0; 
    if(!base_path) base_path = path; 
    DIR *dir = opendir(path); 
    if(!dir){
        fprintf(stderr, "%s: error: could not open directory %s\n", __FUNCTION__, path); 
        return -ENOENT; 
    }
    struct dirent *ent = 0; 
    char fname[255]; 
    while((ent = readdir(dir))){
        if(strcmp(ent->d_name, ".") == 0 || strcmp(ent->d_name, "..") == 0) continue; 

        snprintf(fname, sizeof(fname), "%s/%s", path, ent->d_name); 
        
        if(ent->d_type == DT_DIR) {
            rv |= load_plugins(self, fname, base_path);  
        } else  if(ent->d_type == DT_REG || ent->d_type == DT_LNK){
			// TODO: is there a better way to get filename without extension? 
			char *ext = strrchr(fname, '.');  
			char *name = fname + strlen(base_path); 
			int len = strlen(name); 
			char *objname = alloca( len + 1 ); 
			strncpy(objname, name, len - strlen(ext)); 

			if(strcmp(ext, ".lua") != 0) continue; 
			printf("loading plugin %s of %s at base %s\n", objname, fname, base_path); 
			struct juci_luaobject *obj = juci_luaobject_new(objname); 
			if(juci_luaobject_load(obj, fname) != 0 || avl_insert(&self->objects, &obj->avl) != 0){
				fprintf(stderr, "ERR: could not load plugin %s\n", fname); 
				juci_luaobject_delete(&obj); 
				continue; 
			}
		}
    }
    closedir(dir); 
    return rv; 
}

static void app_init(struct app *self){
	avl_init(&self->objects, avl_strcmp, false, NULL); 
	self->lua = luaL_newstate(); 
	luaL_openlibs(self->lua); 
}

static int app_call(struct app *self, const char *object, const char *method, struct blob_field *args, struct blob *out){
	struct avl_node *avl = avl_find(&self->objects, object); 
	if(!avl) {
		fprintf(stderr, "object not found: %s\n", object); 
		return -ENOENT; 
	}
	struct juci_luaobject *obj = container_of(avl, struct juci_luaobject, avl); 
	return juci_luaobject_call(obj, method, args, out); 
}

int main(int argc, char **argv){
  	const char *www_root = "/www"; 
	int c = 0; 	
	while((c = getopt(argc, argv, "d:")) != -1){
		switch(c){
			case 'd': 
				www_root = optarg; 
				break; 
			default: break; 
		}
	}

    ubus_server_t server = ubus_srv_ws_new(www_root); 
	ubus_client_t client = ubus_cli_js_new(); 

	if(ubus_client_connect(client, "192.168.2.1:1234") < 0){
		fprintf(stderr, "Failed to connect to ubus json socket!\n"); 
		return -1; 
	}

    if(ubus_server_listen(server, "ws://localhost:1234") < 0){
        fprintf(stderr, "server could not listen on specified socket!\n"); 
        return -1;                       
    }

	signal(SIGINT, handle_sigint); 

	struct app app; 
	app_init(&app); 

	load_plugins(&app, "./plugins/", NULL); 
	
	struct blob buf, out; 
	blob_init(&buf, 0, 0); 
	blob_init(&out, 0, 0); 
	blob_put_string(&buf, "key"); 
	blob_put_string(&buf, "value"); 
	if(app_call(&app, "/simple", "print_hello", blob_head(&buf), &out) == 0){
		blob_dump_json(&out); 
	}
	if(app_call(&app, "/subdir/subobj", "print_hello", blob_head(&buf), &out) == 0){
		blob_dump_json(&out); 
	}

	struct ubus_message *msg = ubus_message_new(); 
	blob_put_json(&msg->buf, "{\"jsonrpc\":\"2.0\",\"id\":1,\"method\":\"list\",\"params\":[\"*\"]}"); 
	ubus_client_send(client, &msg); 

	msg = ubus_message_new(); 
	blob_reset(&msg->buf); 
	blob_put_json(&msg->buf, "{\"jsonrpc\":\"2.0\",\"id\":1,\"method\":\"list\",\"params\":[\"*\"]}"); 
	ubus_client_send(client, &msg); 

	uint32_t peer = 0; 
    while(running){                     
        struct ubus_message *msg = NULL;         
		if(ubus_client_recv(client, &msg) > 0){
			//printf("got client message: "); 
			//blob_dump_json(&msg->buf); 
			msg->peer = peer;  
			printf("sending reply to %08x\n", msg->peer); 
			ubus_server_send(server, &msg); 
		}
        if(ubus_server_recv(server, &msg) < 0){  
            continue;                   
        }
        printf("got message from %08x: ", msg->peer); 
        blob_dump_json(&msg->buf);
		peer = msg->peer; 
		ubus_client_send(client, &msg); 
        /*if(ubus_server_send(server, &msg) < 0){  
            printf("could not send echo reply!\n");  
        }*/
    }

	printf("cleaning up\n"); 
	ubus_server_delete(server); 

	return 0; 
}
