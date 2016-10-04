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


#pragma once 

#include <utype/avl.h>

struct orange_user_acl {
	struct avl_node avl; 
}; 

struct orange_user {
	struct avl_node avl; 
	char *username; 
	char *pwhash; 
	struct avl_tree acls; 
	pthread_mutex_t lock; 
}; 

struct orange_user *orange_user_new(const char *username); 
void orange_user_delete(struct orange_user **self); 

void orange_user_add_acl(struct orange_user *self, const char *acl); 
void orange_user_set_pw_hash(struct orange_user *self, const char *pwhash); 

#define orange_user_for_each_acl(user, var) avl_for_each_element(&(user)->acls, var, avl)
