#pragma once

#include "internal.h"

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
