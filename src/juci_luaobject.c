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

#include <dirent.h>

#include "internal.h"
#include "juci_luaobject.h"
#include "juci_lua.h"

#define JUCI_LUA_LIB_PATH "/usr/lib/juci/lib/"

struct juci_luaobject* juci_luaobject_new(const char *name){
	struct juci_luaobject *self = calloc(1, sizeof(struct juci_luaobject)); 
	assert(self); 
	self->lua = luaL_newstate(); 
	self->name = malloc(strlen(name) + 1); 
	strcpy(self->name, name); 
	self->avl.key = self->name; 
	luaL_openlibs(self->lua); 
	blob_init(&self->signature, 0, 0); 

	// add proper lua paths
	lua_getglobal(self->lua, "package"); 
	lua_getfield(self->lua, -1, "path"); 
	char newpath[255];
	const char *dirs[] = {
		getenv("JUCI_LUA_LIB_PATH"),
		"./lualib/",
		JUCI_LUA_LIB_PATH,
	}; 
	const char *lua_libs = 0; 
	for(int c = 0; c < 3; c++){
		lua_libs = dirs[c]; 
		if(!lua_libs) continue; 
		DIR *dir = opendir(lua_libs); 
		if(dir) { closedir(dir); break; }
	}
	if(!lua_libs) lua_libs = "./"; 
	snprintf(newpath, 255, "%s/?.lua;%s/juci/?.lua;%s;?.lua", lua_libs, lua_libs, lua_tostring(self->lua, -1)); 
	//TRACE("LUA: using lua path: %s\n", newpath); 
	lua_pop(self->lua, 1); 
	lua_pushstring(self->lua, newpath); 
	lua_setfield(self->lua, -2, "path"); 
	lua_pop(self->lua, 1); 

	return self; 
}

void juci_luaobject_delete(struct juci_luaobject **self){
	lua_close((*self)->lua); 
	blob_free(&(*self)->signature); 
	free((*self)->name); 
	free(*self); 
	*self = NULL; 
}

int juci_luaobject_load(struct juci_luaobject *self, const char *file){
	if(luaL_loadfile(self->lua, file) != 0){
		ERROR("could not load plugin: %s\n", lua_tostring(self->lua, -1)); 
		return -1; 
	}
	if(lua_pcall(self->lua, 0, 1, 0) != 0){
		ERROR("could not run plugin: %s\n", lua_tostring(self->lua, -1)); 
		return -1; 
	}
	// this just dumps the returned object
	lua_pushnil(self->lua); 
	const char *k; 
	blob_offset_t root = blob_open_table(&self->signature); 
	while(lua_next(self->lua, -2)){
		lua_pop(self->lua, 1); 
		k = lua_tostring(self->lua, -1); 
		blob_put_string(&self->signature, k); 
		blob_offset_t m = blob_open_array(&self->signature); 
		blob_close_array(&self->signature, m); 
	}
	blob_close_table(&self->signature, root); 
	return 0; 
}

int juci_luaobject_call(struct juci_luaobject *self, const char *method, struct blob_field *in, struct blob *out){
	if(!self) return -1; 

	if(lua_type(self->lua, -1) != LUA_TTABLE) {
		ERROR("lua state is broken. No table on stack!\n"); 
		return -1; 
	}

	lua_getfield(self->lua, -1, method); 
	if(!lua_isfunction(self->lua, -1)){
		ERROR("can not call %s on %s: field is not a function!\n", method, self->name); 
		// add an empty object
		blob_offset_t t = blob_open_table(out); 
		blob_close_table(out, t); 
		return -1; 
	}

	if(in) juci_lua_blob_to_table(self->lua, in, true); 
	else lua_newtable(self->lua); 

	if(lua_pcall(self->lua, 1, 1, 0) != 0){
		ERROR("error calling %s: %s\n", method, lua_tostring(self->lua, -1)); 
		return -1; 
	}

	blob_put_string(out, "result"); 
	blob_offset_t t = blob_open_table(out); 
	if(lua_type(self->lua, -1) == LUA_TTABLE) {
		juci_lua_table_to_blob(self->lua, out, true); 
	}
	blob_close_table(out, t); 
	
	lua_pop(self->lua, 1); 	
	return 0; 
}


