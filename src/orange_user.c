/*
	JUCI Backend Server

	Copyright (C) 2016 Martin K. Schröder <mkschreder.uk@gmail.com>

	Based on code by: 
	Copyright (C) 2013 Felix Fietkau <nbd@openwrt.org>
	Copyright (C) 2013-2014 Jo-Philipp Wich <jow@openwrt.org>

	This program is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/


#include <stdlib.h>
#include <string.h>

#include <blobpack/blobpack.h>
#include <libutype/avl-cmp.h>
#include <pthread.h>

#include "orange_user.h"

struct orange_user_cap {
	struct avl_node avl; 
	char *name; 
}; 

struct orange_user_app {
	struct avl_node avl; 
	char *name; 
}; 

struct orange_user *orange_user_new(const char *username){
	struct orange_user *self = calloc(1, sizeof(struct orange_user)); 
	assert(self); 
	self->username = strdup(username); 
	self->avl.key = self->username; 
	avl_init(&self->acls, avl_strcmp, false, NULL); 
	pthread_mutex_init(&self->lock, NULL); 
	return self; 
}

void orange_user_delete(struct orange_user **_self){
	struct orange_user *self = *_self; 	
	if(self->pwhash) free(self->pwhash); 

	struct orange_user_acl *acl, *nacl;
    avl_remove_all_elements(&self->acls, acl, avl, nacl)
		free(acl); 

	free(self->username); 
	pthread_mutex_destroy(&self->lock); 
	free(self); 
	*_self = NULL; 
}

void orange_user_set_pw_hash(struct orange_user *self, const char *pwhash){
	pthread_mutex_lock(&self->lock); 
	if(self->pwhash && strcmp(self->pwhash, pwhash) == 0) {
		pthread_mutex_unlock(&self->lock); 
		return; 
	}
	if(self->pwhash) free(self->pwhash); 
	self->pwhash = strdup(pwhash); 
	pthread_mutex_unlock(&self->lock); 
}

void orange_user_add_acl(struct orange_user *self, const char *acl){
	pthread_mutex_lock(&self->lock); 
	char *name = NULL; 
	struct orange_user_acl *node = calloc_a(sizeof(struct orange_user_acl), &name, strlen(acl)+1); 
	node->avl.key = strcpy(name, acl); 
	avl_insert(&self->acls, &node->avl); 
	pthread_mutex_unlock(&self->lock); 
}


