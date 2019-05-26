/** :ms-top-comment
* +-------------------------------------------------------------------------------+
* |                      ____                  _ _     _                          |
* |                     / ___|_      _____  __| (_)___| |__                       |
* |                     \___ \ \ /\ / / _ \/ _` | / __| '_ \                      |
* |                      ___) \ V  V /  __/ (_| | \__ \ | | |                     |
* |                     |____/ \_/\_/ \___|\__,_|_|___/_| |_|                     |
* |                                                                               |
* |               _____           _              _     _          _               |
* |              | ____|_ __ ___ | |__   ___  __| | __| | ___  __| |              |
* |              |  _| | '_ ` _ \| '_ \ / _ \/ _` |/ _` |/ _ \/ _` |              |
* |              | |___| | | | | | |_) |  __/ (_| | (_| |  __/ (_| |              |
* |              |_____|_| |_| |_|_.__/ \___|\__,_|\__,_|\___|\__,_|              |
* |                                                                               |
* |                       We design hardware and program it                       |
* |                                                                               |
* |               If you have software that needs to be developed or              |
* |                      hardware that needs to be designed,                      |
* |                               then get in touch!                              |
* |                                                                               |
* |                            info@swedishembedded.com                           |
* +-------------------------------------------------------------------------------+
*
*                       This file is part of TheBoss Project
*
* FILE ............... src/main.c
* AUTHOR ............. Martin K. Schröder
* VERSION ............ Not tagged
* DATE ............... 2019-05-26
* GIT ................ https://github.com/mkschreder/orangerpcd
* WEBSITE ............ http://swedishembedded.com
* LICENSE ............ Swedish Embedded Open Source License
*
*          Copyright (C) 2014-2019 Martin Schröder. All rights reserved.
*
* Permission is hereby granted, free of charge, to any person obtaining a copy of
* this software and associated documentation files (the "Software"), to deal in
* the Software without restriction, including without limitation the rights to
* use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
* the Software, and to permit persons to whom the Software is furnished to do so,
* subject to the following conditions:
*
* The above copyright notice and this text, in full, shall be included in all
* copies or substantial portions of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
* FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
* COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
* IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
* CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
**/
#include <time.h>
#include <stdio.h>
#include <unistd.h>
#include <dirent.h>
#include <signal.h>
#include <syslog.h>

#include <utype/avl-cmp.h>

#include "orange.h"
#include "orange_luaobject.h"
#include "orange_ws_server.h"
#include "orange_rpc.h"

pthread_mutex_t runlock; 
pthread_cond_t runcond; 
sig_atomic_t running; 

static void handle_sigint(int sig){
	DEBUG("Interrupted!\n"); 
	running = false; 
	pthread_mutex_lock(&runlock); 
	pthread_cond_signal(&runcond); 
	pthread_mutex_unlock(&runlock); 
}

int main(int argc, char **argv){
	running = true; 

	pthread_mutex_init(&runlock, NULL); 
	pthread_cond_init(&runcond, NULL); 

  	const char *www_root = "/www"; 
	const char *listen_socket = "ws://127.0.0.1:5303"; 
	const char *plugin_dir = "/usr/lib/orange/api/"; 
	const char *pw_file = "/etc/orange/shadow"; 
	const char *acl_dir = "";
	int num_workers = 10; 

	printf("Orange RPCD v%s\n",VERSION); 
	printf("Lua/JSONRPC server\n"); 
	printf("Copyright (c) 2016 Martin Schröder <mkschreder.uk@gmail.com>\n"); 

	setlogmask(LOG_UPTO(LOG_INFO)); 
	openlog("orangerpcd", LOG_CONS | LOG_PID | LOG_NDELAY, LOG_LOCAL1); 

	int c = 0; 	
	while((c = getopt(argc, argv, "d:l:p:vx:a:w:")) != -1){
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
			case 'w':
				num_workers = abs(atoi(optarg)); 
				if(num_workers > 100) 
					printf("WARNING: using more than 100 workers may not make sense!\n"); 
				break; 
			default: break; 
		}
	}

	#if !defined(CONFIG_THREADS)
	num_workers = 0; 
	printf("Note: threading is disabled!\n"); 
	#else
	printf("Threading is enabled! Running with %d workers.\n", num_workers); 
	#endif
	
    orange_server_t server = orange_ws_server_new(www_root); 

    if(orange_server_listen(server, listen_socket) < 0){
        fprintf(stderr, "server could not listen on specified socket!\n"); 
        return -1;                       
    }

	signal(SIGINT, handle_sigint); 
	signal(SIGUSR1, handle_sigint); 

	struct orange *app = orange_new(plugin_dir, pw_file, acl_dir); 

	struct orange_rpc rpc; 
	orange_rpc_init(&rpc, server, app, 5000000UL, num_workers); 

	syslog(LOG_INFO, "orangerpcd jsonrpc server started (%d)", getpid()); 

	#if CONFIG_THREADS
	// wait for abort
	pthread_mutex_lock(&runlock); 
	pthread_cond_wait(&runcond, &runlock); 
	pthread_mutex_unlock(&runlock); 
	#else 
	while(running){
		orange_rpc_process_requests(&rpc); 
	}
	#endif

	DEBUG("cleaning up\n"); 
	orange_rpc_deinit(&rpc); 
	orange_server_delete(server); 
	orange_delete(&app); 

	syslog(LOG_INFO, "orangerpcd jsonrpc server exiting (%d)", getpid()); 

	pthread_mutex_destroy(&runlock); 
	pthread_cond_destroy(&runcond); 

	closelog(); 
	return 0; 
}
