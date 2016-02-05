#pragma once

#include <lua5.2/lua.h>
#include <lua5.2/lauxlib.h>
#include <lua5.2/lualib.h>

#include <blobpack/blobpack.h>
#include <libutype/avl.h>

struct juci_luaobject {
	struct avl_node avl; 
	char *name; 
	lua_State *lua; 
}; 

struct juci_luaobject* juci_luaobject_new(const char *name); 
void juci_luaobject_delete(struct juci_luaobject **self); 
int juci_luaobject_load(struct juci_luaobject *self, const char *file); 
int juci_luaobject_call(struct juci_luaobject *self, const char *method, struct blob_field *in, struct blob *out); 
