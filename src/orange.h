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
#include <pthread.h>

#include "orange_session.h"
#include "json_check.h"

struct orange_message; 

struct orange {
	struct avl_tree objects; 
	struct avl_tree sessions; 
	struct avl_tree users; 
	
	char *plugin_path; 	
	char *pwfile; 
	char *acl_path; 

	pthread_mutex_t lock; 
}; 

struct orange* orange_new(const char *plugin_path, const char *pwfile, const char *acl_dir); 
void orange_delete(struct orange **_self); 

void orange_add_user(struct orange *self, struct orange_user **user); 

int orange_login_plaintext(struct orange *self, const char *username, const char *password, struct orange_sid *sid); 
int orange_login(struct orange *self, const char *username, const char *challenge, const char *response, struct orange_sid *new_sid); 
int orange_logout(struct orange *self, const char *sid); 
bool orange_session_is_valid(struct orange *self, const char *sid); 
//struct orange_session* orange_find_session(struct orange *self, const char *sid); 
int orange_list(struct orange *self, const char *sid, const char *path, struct blob *out); 

int orange_call(struct orange *self, const char *sid, const char *object, const char *method, struct blob_field *args, struct blob *out); 
  

