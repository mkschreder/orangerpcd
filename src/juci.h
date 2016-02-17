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

#pragma once 

#include <blobpack/blobpack.h>
#include <libutype/avl.h>
#include <libutype/avl-cmp.h>

#include "juci_session.h"

struct juci {
	struct avl_tree objects; 
	struct avl_tree sessions; 
	struct avl_tree users; 
	
	struct juci_session *current_session; 
}; 

void juci_init(struct juci *self); 
int juci_load_plugins(struct juci *self, const char *path, const char *base_path); 
int juci_load_passwords(struct juci *self, const char *pwfile); 
int juci_login(struct juci *self, const char *username, const char *challenge, const char *response, const char **new_sid); 
int juci_call(struct juci *self, const char *sid, const char *object, const char *method, struct blob_field *args, struct blob *out); 
int juci_list(struct juci *self, const char *sid, const char *path, struct blob *out); 
   
static inline bool url_scanf(const char *url, char *proto, char *host, int *port, char *page){
    if (sscanf(url, "%99[^:]://%99[^:]:%i/%199[^\n]", proto, host, port, page) == 4) return true; 
    else if (sscanf(url, "%99[^:]://%99[^/]/%199[^\n]", proto, host, page) == 3) return true; 
    else if (sscanf(url, "%99[^:]://%99[^:]:%i[^\n]", proto, host, port) == 3) return true; 
    else if (sscanf(url, "%99[^:]://%99[^\n]", proto, host) == 2) return true; 
    return false;                       
}   

