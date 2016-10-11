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
#include <syslog.h>

#include <utype/avl-cmp.h>

#include "orange.h"
#include "orange_luaobject.h"
#include "orange_ws_server.h"
#include "orange_rpc.h"
#include "orange_eq.h"

static void usage(void){
	printf("Usage: \n"); 
	printf("\torangerpc-client [-q <queue_name>] <broadcast> <event-name> <event-data-json>\n"); 
	exit(-1); 
}

int main(int argc, char **argv){
	const char *queue_name = NULL; // default: orangerpcd-events; 
	int c = 0; 	
	while((c = getopt(argc, argv, "q:")) != -1){
		switch(c){
			case 'q': 
				queue_name = optarg; 
				break; 
			default: 
				usage(); 
				break; 
		}
	}

	if(optind < argc - 3) usage(); 
	
	const char *cmd = argv[optind++]; 	
	const char *name = argv[optind++]; 	
	const char *data = argv[optind++]; 	

	if(!cmd || !name || !data) usage(); 

	struct orange_eq queue; 
	if(orange_eq_open(&queue, queue_name, false) != 0){
		fprintf(stderr, "Unable to open event queue. Check that server is running!\n"); 
		return -1; 
	}

	struct blob b; 
	blob_init(&b, 0, 0); 
	blob_put_string(&b, name); 
	blob_put_json(&b, data); 

	if(orange_eq_send(&queue, &b) != 0){
		fprintf(stderr, "Could not send message!\n"); 
		return -1; 
	}
	
	blob_free(&b); 
	orange_eq_close(&queue); 

	return 0; 
}
