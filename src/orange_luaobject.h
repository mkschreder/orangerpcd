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

#include "internal.h"

#include <blobpack/blobpack.h>
#include <libutype/avl.h>

struct orange_session; 

struct orange_luaobject {
	struct avl_node avl; 
	char *name; 
	struct blob signature; 
	lua_State *lua; 
	pthread_mutex_t lock; 
}; 

struct orange_luaobject* orange_luaobject_new(const char *name); 
void orange_luaobject_delete(struct orange_luaobject **self); 
int orange_luaobject_load(struct orange_luaobject *self, const char *file); 
int orange_luaobject_call(struct orange_luaobject *self, struct orange_session *ses, const char *method, struct blob_field *in, struct blob *out); 
