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

#include <blobpack/blobpack.h>

#include "internal.h"
#include "juci_lua.h"

void juci_lua_blob_to_table(lua_State *lua, struct blob_field *msg, bool table){
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
				juci_lua_blob_to_table(lua, child, false); 
				break; 
			case BLOB_FIELD_TABLE: 
				juci_lua_blob_to_table(lua, child, true); 
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


int juci_lua_table_to_blob(lua_State *L, struct blob *b, bool table){
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
					rv = juci_lua_table_to_blob(L, b, false);
					blob_close_array(b, c);
				} else {
					blob_offset_t c = blob_open_table(b);
					rv = juci_lua_table_to_blob(L, b, true);
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

static int l_json_parse(lua_State *L){
	const char *str = lua_tostring(L, 1); 
	struct blob tmp; 
	blob_init(&tmp, 0, 0); 
	blob_put_json(&tmp, str); 	
	if(juci_debug_level >= JUCI_DBG_TRACE){
		TRACE("lua blob: "); 
		blob_dump_json(&tmp); 
	}
	juci_lua_blob_to_table(L, blob_field_first_child(blob_head(&tmp)), true);
	blob_free(&tmp); 
	return 1; 
}

void juci_lua_publish_json_api(lua_State *L){
	// add fast json parsing
	lua_newtable(L); 
	lua_pushstring(L, "parse"); 
	lua_pushcfunction(L, l_json_parse); 
	lua_settable(L, -3); 
	lua_setglobal(L, "JSON"); 
}

