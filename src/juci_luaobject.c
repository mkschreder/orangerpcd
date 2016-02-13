#include "juci_luaobject.h"

struct juci_luaobject* juci_luaobject_new(const char *name){
	struct juci_luaobject *self = calloc(1, sizeof(struct juci_luaobject)); 
	self->lua = luaL_newstate(); 
	self->name = malloc(strlen(name) + 1); 
	strcpy(self->name, name); 
	self->avl.key = self->name; 
	luaL_openlibs(self->lua); 
	return self; 
}

void juci_luaobject_delete(struct juci_luaobject **self){
	lua_close((*self)->lua); 
	free(*self); 
	*self = NULL; 
}

int juci_luaobject_load(struct juci_luaobject *self, const char *file){
	if(luaL_loadfile(self->lua, file) != 0){
		fprintf(stderr, "could not load plugin: %s\n", lua_tostring(self->lua, -1)); 
		return -1; 
	}
	if(lua_pcall(self->lua, 0, 1, 0) != 0){
		fprintf(stderr, "could not run plugin: %s\n", lua_tostring(self->lua, -1)); 
		return -1; 
	}
	// this just dumps the returned object
	lua_pushnil(self->lua); 
	const char *k, *v; 
	while(lua_next(self->lua, -2)){
		v = lua_tostring(self->lua, -1); 
		lua_pop(self->lua, 1); 
		k = lua_tostring(self->lua, -1); 
		printf("return k=>%s v=>%s\n", k, v); 
	}
/*	
	lua_getfield(self->lua, -1, "print_hello"); 
	lua_pcall(self->lua, 0, 1, 0); 
	lua_pop(self->lua, 1); 

	lua_getfield(self->lua, -1, "print_hello"); 
	lua_pcall(self->lua, 0, 1, 0); 


	// call function
	lua_getglobal(self->lua, "print_hello"); 
	lua_newtable(self->lua); 
	lua_pushliteral(self->lua, "test"); 
	lua_pushliteral(self->lua, "This is a string"); 
	lua_settable(self->lua, -3); 
	if(lua_pcall(self->lua, 1, 1, 0)){
		fprintf(stderr, "plugin init failed for %s\n", fname); 
	}
	lua_pushnil(self->lua); 
	while(lua_next(self->lua, -2)){
		v = lua_tostring(self->lua, -1); 
		lua_pop(self->lua, 1); 
		k = lua_tostring(self->lua, -1); 
		printf("return k=>%s v=>%s\n", k, v); 
	}
	*/
	return 0; 
}

static void _lua_pushblob(lua_State *lua, struct blob_field *msg){
	int nc = -1; 
	lua_newtable(lua); 
	struct blob_field *key, *child; 
	blob_field_for_each_kv(msg, key, child){
		lua_pushstring(lua, blob_field_get_string(key)); 
		switch(blob_field_type(child)){
			case BLOB_FIELD_INT8: 
			case BLOB_FIELD_INT16: 
			case BLOB_FIELD_INT32: 
				lua_pushinteger(lua, blob_field_get_int(child)); 
				break; 
			case BLOB_FIELD_STRING: 
				lua_pushstring(lua, blob_field_get_string(child)); 
				break; 
			default: 
				lua_pushnil(lua); 	
				break; 
		}
		nc-=2; 
	}
	lua_settable(lua, nc); 
}

int juci_luaobject_call(struct juci_luaobject *self, const char *method, struct blob_field *in, struct blob *out){
	lua_getfield(self->lua, -1, method); 
	if(!lua_isfunction(self->lua, -1)){
		fprintf(stderr, "error calling %s on %s: field is not a function!\n", method, self->name); 
		return -1; 
	}
	if(in){
		_lua_pushblob(self->lua, in); 
	}
	if(lua_pcall(self->lua, (in)?1:0, (out)?1:0, 0) != 0){
		fprintf(stderr, "error calling %s: %s\n", method, lua_tostring(self->lua, -1)); 
		return -1; 
	}

	if(out){
		lua_pushnil(self->lua); 
		while(lua_next(self->lua, -2)){
			if(lua_isstring(self->lua, 0)){
				const char *v = lua_tostring(self->lua, -1); lua_pop(self->lua, 1);  
				const char *k = lua_tostring(self->lua, -1); 
				if(k && v){
					blob_put_string(out, k); 
					blob_put_string(out, v); 
				}
			} else {
				lua_pop(self->lua, 2);  
				//lua_pop(self->lua, 1); 
			}
		}
	}
	return 0; 
}
