#include "test-funcs.h"
#include <stdbool.h>
#include <math.h>
#include <memory.h>
#include <blobpack/blobpack.h>
#include <stdio.h>
#include <unistd.h>

#include "../src/orange.h"
#include "../src/orange_ws_server.h"
#include "../src/orange_rpc.h"
#include "../src/internal.h"

int main(){
	orange_debug_level+=2; 

	const char *listen_socket = "ws://localhost:61413"; 
	orange_server_t server = orange_ws_server_new("test-www"); 
	struct orange *app = orange_new("test-plugins", "test-pwfile", "test-acls");
	struct orange_rpc rpc; 
	orange_rpc_init(&rpc); 

	// add admin user
	struct orange_user *admin = orange_user_new("admin"); 
	orange_user_add_acl(admin, "test-acl"); 
	orange_add_user(app, &admin); 

	// fork off a test client
	if(fork() == 0){
		sleep(1); 
		int ret = system("./ws_server_tests.sh"); 
		exit(ret); 
	} 

    if(orange_server_listen(server, listen_socket) < 0){
        fprintf(stderr, "server could not listen on specified socket!\n"); 
        return -1;                       
    }

	while(1){
		int ret = orange_rpc_process_requests(&rpc, server, app, 5000000UL); 
		if(ret == -ETIMEDOUT){
			printf("Timed out while waiting for request!\n"); 
			exit(1); 
		}
	}
	
	orange_server_delete(server); 
	orange_delete(&app); 

	return 0; 
}
