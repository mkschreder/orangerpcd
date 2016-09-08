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

#include <time.h>
#include <stdio.h>
#include <unistd.h>
#include <dirent.h>
#include <signal.h>

#include <libutype/avl-cmp.h>

#include "orange.h"
#include "orange_luaobject.h"
#include "orange_ws_server.h"
#include "orange_rpc.h"

pthread_mutex_t runlock; 
pthread_cond_t runcond; 
static void handle_sigint(int sig){
	DEBUG("Interrupted!\n"); 
	pthread_mutex_lock(&runlock); 
	pthread_cond_signal(&runcond); 
	pthread_mutex_unlock(&runlock); 
}

int main(int argc, char **argv){
	pthread_mutex_init(&runlock, NULL); 
	pthread_cond_init(&runcond, NULL); 

  	const char *www_root = "/www"; 
	const char *listen_socket = "ws://127.0.0.1:5303"; 
	const char *plugin_dir = "/usr/lib/orange/api/"; 
	const char *pw_file = "/etc/orange/shadow"; 
	const char *acl_dir = "";

	printf("Orange RPCD v%s\n",VERSION); 
	printf("Lua/JSONRPC server\n"); 
	printf("Copyright (c) 2016 Martin Schröder <mkschreder.uk@gmail.com>\n"); 

	int c = 0; 	
	while((c = getopt(argc, argv, "d:l:p:vx:a:")) != -1){
		switch(c){
			case 'd': 
				www_root = optarg; 
				break; 
			case 'a': 
				acl_dir = optarg; 
				break; 
			case 'l':
				listen_socket = optarg; 
				break; 
			case 'p': 
				plugin_dir = optarg; 
				break; 
			case 'v': 
				orange_debug_level++; 
				break; 
			case 'x': 
				pw_file = optarg; 
				break; 
			default: break; 
		}
	}

    orange_server_t server = orange_ws_server_new(www_root); 

    if(orange_server_listen(server, listen_socket) < 0){
        fprintf(stderr, "server could not listen on specified socket!\n"); 
        return -1;                       
    }

	signal(SIGINT, handle_sigint); 
	signal(SIGUSR1, handle_sigint); 

	struct orange *app = orange_new(plugin_dir, pw_file, acl_dir); 

	struct orange_rpc rpc; 
	orange_rpc_init(&rpc, server, app, 10000UL, 1); 

	// wait for abort
	pthread_mutex_lock(&runlock); 
	pthread_cond_wait(&runcond, &runlock); 
	pthread_mutex_unlock(&runlock); 

	DEBUG("cleaning up\n"); 
	orange_rpc_deinit(&rpc); 
	orange_server_delete(server); 
	orange_delete(&app); 

	pthread_mutex_destroy(&runlock); 
	pthread_cond_destroy(&runcond); 

	return 0; 
}
