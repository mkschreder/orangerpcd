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
* FILE ............... src/orange_luaobject.c
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
#include <dirent.h>
#include <pthread.h>

#include "internal.h"
#include "orange_luaobject.h"
#include "orange_lua.h"

#define JUCI_LUA_LIB_PATH "/usr/lib/orange/lib/"

static lua_State * _luaobject_create_lua_state(void){
	lua_State *L = luaL_newstate(); 		
	// export server api to the lua object
	orange_lua_publish_json_api(L); 
	orange_lua_publish_file_api(L); 
	orange_lua_publish_session_api(L); 
	orange_lua_publish_core_api(L); 
	return L; 
}

struct orange_luaobject* orange_luaobject_new(const char *name){
	struct orange_luaobject *self = calloc(1, sizeof(struct orange_luaobject)); 
	assert(self); 
	self->lua = _luaobject_create_lua_state(); 
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
	snprintf(newpath, 255, "%s/?.lua;%s/orange/?.lua;%s;?.lua", lua_libs, lua_libs, lua_tostring(self->lua, -1)); 
	//TRACE("LUA: using lua path: %s\n", newpath); 
	lua_pop(self->lua, 1); 
	lua_pushstring(self->lua, newpath); 
	lua_setfield(self->lua, -2, "path"); 
	lua_pop(self->lua, 1); 

	pthread_mutex_init(&self->lock, NULL); 

	return self; 
}

void orange_luaobject_free_state(struct orange_luaobject *self){
	if(!self->lua) return; 
	lua_close(self->lua); 
	self->lua = 0; 
}

void orange_luaobject_delete(struct orange_luaobject **self){
	orange_luaobject_free_state(*self); 
	blob_free(&(*self)->signature); 
	free((*self)->name); 
	pthread_mutex_destroy(&(*self)->lock); 
	free(*self); 
	*self = NULL; 
}

int orange_luaobject_load(struct orange_luaobject *self, const char *file){
	pthread_mutex_lock(&self->lock); 

	if(!self->lua) self->lua = _luaobject_create_lua_state();  
	if(luaL_loadfile(self->lua, file) != 0){
		ERROR("could not load plugin: %s\n", lua_tostring(self->lua, -1)); 
		pthread_mutex_unlock(&self->lock); 
		return -1; 
	}
	if(lua_pcall(self->lua, 0, 1, 0) != 0){
		ERROR("could not run plugin: %s\n", lua_tostring(self->lua, -1)); 
		pthread_mutex_unlock(&self->lock); 
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

	pthread_mutex_unlock(&self->lock); 
	return 0; 
}

int orange_luaobject_call(struct orange_luaobject *self, struct orange_session *session, const char *method, const struct blob_field *in, struct blob *out){
	if(!self || !self->lua) return -1; 
	
	char errbuf[255];

	pthread_mutex_lock(&self->lock); 
	// set self pointer of the global lua session object to point to current session
	orange_lua_set_session(self->lua, session); 

	// first item on lua stack should always be the table that was returned when the file was run at load time
	if(lua_type(self->lua, -1) != LUA_TTABLE) {
		ERROR("lua state is broken. No table on stack!\n");

		blob_put_string(out, "error"); 
		blob_offset_t t = blob_open_table(out); 
		blob_put_string(out, "str"); 
		snprintf(errbuf, sizeof(errbuf), "Lua backend state is broken! This should never happen!"); // TODO: get some kind of description from lua?
		blob_put_string(out, errbuf);
		blob_put_string(out, "code");
		blob_put_int(out, lua_tointeger(self->lua, -1));
		blob_close_table(out, t); 

		pthread_mutex_unlock(&self->lock); 
		return -1; 
	}

	// we get the field indexed by supplied method from our lua object
	lua_getfield(self->lua, -1, method); 
	if(!lua_isfunction(self->lua, -1)){
		snprintf(errbuf, sizeof(errbuf), "Can not call %s on %s: field is not a function!", method, self->name);
		ERROR("%s\n", errbuf);

		blob_put_string(out, "error"); 
		blob_offset_t t = blob_open_table(out); 
		blob_put_string(out, "str"); 
		blob_put_string(out, errbuf);
		blob_put_string(out, "code"); 
		blob_put_int(out, lua_tointeger(self->lua, -1));
		blob_close_table(out, t); 

		pthread_mutex_unlock(&self->lock); 
		return -1; 
	}

	// arguments are either supplied by the user or an empty table
	if(in) orange_lua_blob_to_table(self->lua, in, true); 
	else lua_newtable(self->lua); 

	// call the method that we previously poped out of the table
	if(lua_pcall(self->lua, 1, 1, 0) != 0){
		snprintf(errbuf, sizeof(errbuf), "error calling %s: %s", method, lua_tostring(self->lua, -1));
		ERROR("%s\n", errbuf);

		blob_put_string(out, "error"); 
		blob_offset_t t = blob_open_table(out); 
		blob_put_string(out, "str"); 
		blob_put_string(out, errbuf); 
		blob_put_string(out, "code"); 
		blob_put_int(out, lua_tointeger(self->lua, -1));
		blob_close_table(out, t); 
		pthread_mutex_unlock(&self->lock); 
		return -1; 
	}

	// support both table response and an error code response. 
	// if lua method returns a number then it is always treated as an error code
	if(lua_type(self->lua, -1) == LUA_TTABLE) {
		blob_put_string(out, "result"); 
		blob_offset_t t = blob_open_table(out); 
		orange_lua_table_to_blob(self->lua, out, true); 
		blob_close_table(out, t); 
	} else if(lua_type(self->lua, -1) == LUA_TNUMBER){
		blob_put_string(out, "error"); 
		blob_offset_t t = blob_open_table(out); 
		blob_put_string(out, "str");
		blob_put_string(out, "Lua function returned error."); // if your intention is to return a success, always return an object. Even an empty one: {}
		blob_put_string(out, "code"); 
		blob_put_int(out, lua_tointeger(self->lua, -1));
		blob_close_table(out, t); 
	} else {
		// for all other return types return an empty object
		blob_put_string(out, "result"); 
		blob_offset_t t = blob_open_table(out); 
		blob_close_table(out, t); 
	}

	lua_pop(self->lua, 1); 	
	
	pthread_mutex_unlock(&self->lock); 
	return 0; 
}


