#include "internal.h"
#include "juci_luaobject.h"

static int l_json_parse(lua_State *L);

#define JUCI_LUA_LIB_PATH "/usr/lib/juci/lib/"

struct juci_luaobject* juci_luaobject_new(const char *name){
	struct juci_luaobject *self = calloc(1, sizeof(struct juci_luaobject)); 
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
	char *lua_libs = getenv("JUCI_LUA_LIB_PATH"); 
	if(!lua_libs) lua_libs = JUCI_LUA_LIB_PATH; 
	snprintf(newpath, 255, "%s/?.lua;%s/juci/?.lua;%s;?.lua", lua_libs, lua_libs, lua_tostring(self->lua, -1)); 
	TRACE("LUA: using lua path: %s\n", newpath); 
	lua_pop(self->lua, 1); 
	lua_pushstring(self->lua, newpath); 
	lua_setfield(self->lua, -2, "path"); 
	lua_pop(self->lua, 1); 
	
	// add fast json parsing
	lua_newtable(self->lua); 
	lua_pushstring(self->lua, "parse"); 
	lua_pushcfunction(self->lua, l_json_parse); 
	lua_settable(self->lua, -3); 
	lua_setglobal(self->lua, "JSON"); 

	return self; 
}

void juci_luaobject_delete(struct juci_luaobject **self){
	lua_close((*self)->lua); 
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

static void _lua_pushblob(lua_State *lua, struct blob_field *msg, bool table){
	lua_newtable(lua); 
	struct blob_field *child; 
	int index = 1; 

	blob_field_for_each_child(msg, child){
		if(table) {
			lua_pushstring(lua, blob_field_get_string(child)); 
			child = blob_field_next_child(msg, child); 
		} else {
			lua_pushnumber(lua, index); 
			index++; 
		}
		switch(blob_field_type(child)){
			case BLOB_FIELD_INT8: 
			case BLOB_FIELD_INT16: 
			case BLOB_FIELD_INT32: 
				lua_pushinteger(lua, blob_field_get_int(child)); 
				break; 
			case BLOB_FIELD_STRING: 
				lua_pushstring(lua, blob_field_get_string(child)); 
				break; 
			case BLOB_FIELD_ARRAY: 
				_lua_pushblob(lua, child, false); 
				break; 
			case BLOB_FIELD_TABLE: 
				_lua_pushblob(lua, child, true); 
				break; 
			default: 
				lua_pushnil(lua); 	
				break; 
		}
		lua_settable(lua, -3); 
	}
}

static bool _lua_format_blob_is_array(lua_State *L){
	lua_Integer prv = 0;
	lua_Integer cur = 0;

	lua_pushnil(L); 
	while(lua_next(L, -2)){
#ifdef LUA_TINT
		if (lua_type(L, -2) != LUA_TNUMBER && lua_type(L, -2) != LUA_TINT)
#else
		if (lua_type(L, -2) != LUA_TNUMBER)
#endif
		{
			lua_pop(L, 2);
			return false;
		}

		cur = lua_tointeger(L, -2);

		if ((cur - 1) != prv){
			lua_pop(L, 2);
			return false;
		}

		prv = cur;
		lua_pop(L, 1); 
	}

	return true;
}

static int _lua_format_blob(lua_State *L, struct blob *b, bool table){
	bool rv = true;

	if(lua_type(L, -1) != LUA_TTABLE) {
		DEBUG("%s: can only format a table (or array)\n", __FUNCTION__); 
		return false; 
	}

	lua_pushnil(L); 
	while(lua_next(L, -2)){
		lua_pushvalue(L, -2); 

		const char *key = table ? lua_tostring(L, -1) : NULL;

		if(key) blob_put_string(b, key); 

		switch (lua_type(L, -2)){
			case LUA_TBOOLEAN:
				blob_put_int(b, (uint8_t)lua_toboolean(L, -2));
				break;
		#ifdef LUA_TINT
			case LUA_TINT:
		#endif
			case LUA_TNUMBER:
				blob_put_int(b, (uint32_t)lua_tointeger(L, -2));
				break;
			case LUA_TSTRING:
			case LUA_TUSERDATA:
			case LUA_TLIGHTUSERDATA:
				blob_put_string(b, lua_tostring(L, -2));
				break;
			case LUA_TTABLE:
				lua_pushvalue(L, -2); 
				if (_lua_format_blob_is_array(L)){
					blob_offset_t c = blob_open_array(b);
					rv = _lua_format_blob(L, b, false);
					blob_close_array(b, c);
				} else {
					blob_offset_t c = blob_open_table(b);
					rv = _lua_format_blob(L, b, true);
					blob_close_table(b, c);
				}
				// pop the value of the table we pushed earlier
				lua_pop(L, 1); 
				break;
			default:
				rv = false;
				break;
		}
		// pop both the value we pushed and the item require to be poped after calling next
		lua_pop(L, 2); 
	}
	//lua_pop(L, 1); 
	return rv;
}

int juci_luaobject_call(struct juci_luaobject *self, const char *method, struct blob_field *in, struct blob *out){
	if(!self) return -1; 

	lua_getfield(self->lua, -1, method); 
	if(!lua_isfunction(self->lua, -1)){
		ERROR("can not call %s on %s: field is not a function!\n", method, self->name); 
		// add an empty object
		blob_offset_t t = blob_open_table(out); 
		blob_close_table(out, t); 
		return -1; 
	}

	if(in) _lua_pushblob(self->lua, in, true); 
	else lua_newtable(self->lua); 

	if(lua_pcall(self->lua, 1, 1, 0) != 0){
		ERROR("error calling %s: %s\n", method, lua_tostring(self->lua, -1)); 
		return -1; 
	}

	blob_offset_t t = blob_open_table(out); 
	_lua_format_blob(self->lua, out, true); 
	blob_close_table(out, t); 
	
	lua_pop(self->lua, 1); 	
	return 0; 
}

static int l_json_parse(lua_State *L){
	const char *str = lua_tostring(L, 1); 
	struct blob tmp; 
	blob_init(&tmp, 0, 0); 
	blob_put_json(&tmp, str); 	
	if(juci_debug_level >= JUCI_DBG_TRACE){
		TRACE("lua blob: "); 
		blob_dump_json(&tmp); 
	}
	_lua_pushblob(L, blob_field_first_child(blob_head(&tmp)), true); 	
	blob_free(&tmp); 
	return 1; 
}


