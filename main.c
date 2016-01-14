#include <stdio.h>
#include <libubus2/libubus2.h>
#include <libubus2/json_websocket.h>
#include <libubus2/json_socket.h>

bool running = true; 

void _on_message(ubus_socket_t socket, uint32_t peer, uint8_t type, uint32_t serial, struct blob_field *msg){ 
	printf("got %s\n", ubus_message_types[type]); 
	blob_field_dump_json(msg); 
}

void handle_sigint(){
	printf("Interrupted!\n"); 
	running = false; 
}

int main(int argc, char **argv){
	ubus_socket_t insock = json_websocket_new(); 
	ubus_socket_t outsock = json_socket_new(); 
	struct ubus_proxy *proxy = ubus_proxy_new(&insock, &outsock); 
	ubus_proxy_listen(proxy, "localhost:1234"); 
	ubus_proxy_connect(proxy, "/var/run/ubus-json.sock"); 

	signal(SIGINT, handle_sigint); 

	while(running){
		ubus_proxy_handle_events(proxy); 
	}
	
	printf("cleaning up\n"); 
	ubus_proxy_delete(&proxy); 

	return 0; 
}
