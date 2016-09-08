#include "test-funcs.h"
#include <stdbool.h>
#include <math.h>
#include <memory.h>
#include <blobpack/blobpack.h>
#include <stdio.h>
#include <unistd.h>
#include <signal.h>

#include "../src/orange.h"
#include "../src/orange_ws_server.h"
#include "../src/orange_rpc.h"
#include "../src/internal.h"

static orange_server_t server = NULL; 
static struct orange *app = NULL; 
static struct orange_rpc rpc; 

pthread_mutex_t runlock; 
pthread_cond_t runcond; 
void _cleanup(int sig){
	pthread_mutex_lock(&runlock); 
	pthread_cond_signal(&runcond); 
	pthread_mutex_unlock(&runlock); 
}

int main(){
	orange_debug_level+=4; 

	pthread_mutex_init(&runlock, NULL); 
	pthread_cond_init(&runcond, NULL); 

	// fork off a test client
	if(fork() == 0){
		sleep(1); 
		int ret = system("./ws_server_tests.sh"); 
		exit(ret); 
	}

	const char *listen_socket = "ws://127.0.0.1:61413"; 
	server = orange_ws_server_new("test-www"); 
	app = orange_new("test-plugins", "test-pwfile", "test-acls");

	// add admin user
	struct orange_user *admin = orange_user_new("admin"); 
	orange_user_add_acl(admin, "test-acl"); 
	orange_add_user(app, &admin); 

	// test scanurl and also test invalid socket connection
	if(orange_server_listen(server, "asdf://localhostasd") >= 0) return -1; 
    if(orange_server_listen(server, listen_socket) < 0){
        fprintf(stderr, "server could not listen on specified socket!\n"); 
        return -1;                       
    }

	orange_rpc_init(&rpc, server, app, 10000UL, 10); 

	signal(SIGINT, _cleanup); 
	signal(SIGUSR1, _cleanup); 

	// will wait until lock is unlocked
	pthread_mutex_lock(&runlock); 
	pthread_cond_wait(&runcond, &runlock); 
	pthread_mutex_unlock(&runlock); 

	printf("cleaning up..\n"); 
	orange_rpc_deinit(&rpc); 
	orange_server_delete(server); 
	orange_delete(&app); 

	pthread_mutex_destroy(&runlock); 
	pthread_cond_destroy(&runcond); 

	return 0; 
}
