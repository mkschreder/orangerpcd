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
* FILE ............... src/orange_lua.c
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
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <unistd.h>
#include <signal.h>
#include <math.h>
#include <float.h>

#include <utype/utils.h>
#include <blobpack/blobpack.h>

#include "internal.h"
#include "orange_lua.h"
#include "orange_session.h"

void orange_lua_blob_to_table(lua_State *lua, const struct blob_field *msg, bool table){
	lua_newtable(lua); 

	const struct blob_field *child; 
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
				orange_lua_blob_to_table(lua, child, false); 
				break; 
			case BLOB_FIELD_TABLE: 
				orange_lua_blob_to_table(lua, child, true); 
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


int orange_lua_table_to_blob(lua_State *L, struct blob *b, bool table){
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
				if(fabsf(lua_tointeger(L, -2) - lua_tonumber(L, -2)) < FLT_EPSILON)
					blob_put_int(b, lua_tointeger(L, -2)); 
				else 
					blob_put_real(b, lua_tonumber(L, -2));
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
					rv = orange_lua_table_to_blob(L, b, false);
					blob_close_array(b, c);
				} else {
					blob_offset_t c = blob_open_table(b);
					rv = orange_lua_table_to_blob(L, b, true);
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
	if(!blob_put_json(&tmp, str)){
		// put emtpy object if json was invalid!
		blob_offset_t b = blob_open_table(&tmp); 
		blob_close_table(&tmp, b); 
	}
	/*
	if(orange_debug_level >= JUCI_DBG_TRACE){
		TRACE("lua blob: "); 
		blob_dump_json(&tmp); 
	}
	*/
	orange_lua_blob_to_table(L, blob_field_first_child(blob_head(&tmp)), true);
	blob_free(&tmp); 
	return 1; 
}

void orange_lua_publish_json_api(lua_State *L){
	// add fast json parsing
	lua_newtable(L); 
	lua_pushstring(L, "parse"); 
	lua_pushcfunction(L, l_json_parse); 
	lua_settable(L, -3); 
	lua_setglobal(L, "JSON"); 
}

#include "base64.h"

static int l_file_write_fragment(lua_State *L){
	int n = lua_gettop(L); 
	if(n != 4 || !lua_isstring(L, 1) || !lua_isnumber(L, 2) || !lua_isnumber(L, 3) || !lua_isstring(L, 4)){
		ERROR("invalid arguments to %s\n", __FUNCTION__); 
		return -1; 
	}
	const char *file = luaL_checkstring(L, 1); 
	long unsigned int offset = luaL_checkinteger(L, 2); 
	long unsigned int len = lua_tointeger(L, 3); 
	const char *data = luaL_checkstring(L, 4); 
	
	// write to the file (note: this is very innefficient but it is mostly just a proof of concept. 
	int flags = O_WRONLY | O_CREAT; 
	if(offset == 0) flags |= O_TRUNC; 
	int fd = open(file, flags, 755); 
	if(fd <= 0) {
		ERROR("could not open file '%s' for writing!\n", file); 
		lua_pop(L, n); 
		return -1; 
	}
	lseek(fd, offset, SEEK_SET); 
	int in_size = strlen(data); 
	int bin_size = in_size * 2;  
	char *bin = malloc(bin_size);
	assert(bin); 
	int size = base64_decode(data, bin, bin_size);   

	TRACE("writing %d bytes at offset %d to file. (supplied length: %lu)\n", (int)size, (int)offset, len); 
	if(size != write(fd, bin, size)){
		ERROR("could not write data to file!\n"); 
		lua_pop(L, n); 
		close(fd); 
		free(bin); 
		return -1; 
	}
	free(bin); 
	close(fd); 
	lua_pop(L, n); 
	return 0; 
}

void orange_lua_publish_file_api(lua_State *L){
	// add fast json parsing
	lua_newtable(L); 
	lua_pushstring(L, "writeFragment"); 
	lua_pushcfunction(L, l_file_write_fragment); 
	lua_settable(L, -3); 
	lua_setglobal(L, "fs"); 
}

static struct orange_session *l_get_session_ptr(lua_State *L){
	lua_getglobal(L, "SESSION"); 
	luaL_checktype(L, -1, LUA_TTABLE); 
	lua_getfield(L, -1, "_self"); 
	struct orange_session *self = (struct orange_session *)lua_touserdata(L, -1); 
	lua_pop(L, 2); // pop JUCI and _self
	if(!self){
		ERROR("Invalid SESSION._self pointer!\n"); 
		return NULL; 
	}
	return self; 
}

// SESSION.access(scope, object, method, permission)
static int l_session_access(lua_State *L){
	struct orange_session *self = l_get_session_ptr(L); 
	if(!self){
		lua_pushboolean(L, false); 
		return 1; 
	}
	const char *scope = luaL_checkstring(L, 1);  
	const char *obj = luaL_checkstring(L, 2);  
	const char *method = luaL_checkstring(L, 3);  
	const char *perm = luaL_checkstring(L, 4);  
	TRACE("checking access to %s %s %s %s\n", scope, obj, method, perm); 
	if(scope && obj && method && perm && orange_session_access(self, scope, obj, method, perm)){
		lua_pushboolean(L, true); 
		return 1; 
	} 
	lua_pushboolean(L, false); 
	return 1; 
}

static int l_session_get(lua_State *L){
	struct orange_session *self = l_get_session_ptr(L); 
	lua_newtable(L); 
	if(!self) return 1; 
	lua_pushstring(L, "username"); lua_pushstring(L, self->user->username); lua_settable(L, -3); 
	return 1; 
}

static int l_core_fork_shell(lua_State *L){
	const char *cmd = luaL_checkstring(L, 1); 
	if(!cmd) {
		lua_pushboolean(L, false); 
		return 1; 
	}

	if(fork() == 0){
		int r = system(cmd); 
		exit(r); 
	} 

	lua_pushboolean(L, true); 
	return 1; 
}

// ++ DEFERRED SHELL IMPLEMENTATION
#include <pthread.h>
#include <utype/avl.h>
#include <utype/avl-cmp.h>
#include <sys/prctl.h>

#include "util.h"
static struct avl_tree _deferred_commands; 
static pthread_mutex_t _deferred_lock = PTHREAD_MUTEX_INITIALIZER; 
static pthread_cond_t _deferred_cond = PTHREAD_COND_INITIALIZER; 
static pthread_t _deferred_thread; 
static bool _deferred_running = false; 

struct deferred_command {
	struct avl_node avl; 
	struct timespec ts_run; 
	char *command; 
}; 

static void *_deferred_worker(void *ptr){
	prctl(PR_SET_NAME, "deferred_shell"); 
	pthread_mutex_lock(&_deferred_lock); 
	while(_deferred_running){
		struct deferred_command *cmd, *tmp; 
		struct timespec ts_sleep_until; 
		struct timespec ts_now; 
		
		timespec_now(&ts_now); 

		// set maximum timeout to 1 sec
		timespec_from_now_us(&ts_sleep_until, 1000000UL); 

		// go through the list and either run commands that need to run 
		// or update our timeout to the time we need to sleep until next command needs to run
		avl_for_each_element_safe(&_deferred_commands, cmd, avl, tmp){
			if(timespec_before(&cmd->ts_run, &ts_now)){
				int __attribute__((unused)) ret = system(cmd->command); 

				// remove the command from the list
				avl_delete(&_deferred_commands, &cmd->avl); 
				free(cmd->command); 
				free(cmd); 
			} else if(timespec_before(&cmd->ts_run, &ts_sleep_until)){
				memcpy(&ts_sleep_until, &cmd->ts_run, sizeof(struct timespec)); 
			}
		}

		// put us into interruptible sleep with mutex unlocked until timeout expires
		// sleep can be interrupted by a new command being added into the list  
		// if sleep is interrupted for any other reason, then we check if timeout expired and go back to sleep if it has not.  
		while(_deferred_running && avl_size(&_deferred_commands) == 0 && !timespec_expired(&ts_sleep_until)){
			// unlock mutex and wait for new messages until it's time to do some processing
			pthread_cond_timedwait(&_deferred_cond, &_deferred_lock, &ts_sleep_until); 
		}
	}
	pthread_mutex_unlock(&_deferred_lock); 
	return NULL; 
}

static void _orange_lua_start_deferred_queue(void){
	avl_init(&_deferred_commands, avl_strcmp, false, NULL);
	_deferred_running = true; 
	pthread_create(&_deferred_thread, NULL, _deferred_worker, NULL); 
}

static void _orange_lua_stop_deferred_queue(void){
	pthread_mutex_lock(&_deferred_lock); 
	_deferred_running = false; 
	pthread_mutex_unlock(&_deferred_lock); 

	pthread_join(_deferred_thread, NULL); 

	// deferred queue is always empty when we get here 
}

static int l_core_deferred_shell(lua_State *L){
	const char *cmd = luaL_checkstring(L, 1); 
	const uint32_t timeout = luaL_checkinteger(L, 2) * 1000;  // argument is in ms

	if(!cmd) {
		lua_pushboolean(L, false); 
		return 1; 
	}

	pthread_mutex_lock(&_deferred_lock); 

	// if the deferred queue is not running then we start it here
	if(!_deferred_running){
		_orange_lua_start_deferred_queue(); 
		atexit(&_orange_lua_stop_deferred_queue); 
	}

	struct deferred_command *cm = NULL;  
	if((cm = avl_find_element(&_deferred_commands, cmd, cm, avl))){
		// update timeout and exit
		TRACE("Found existing entry for deferred command [%s].. updating timeout to %d.\n", cmd, timeout); 
		timespec_from_now_us(&cm->ts_run, timeout); 
		pthread_cond_signal(&_deferred_cond); 
		pthread_mutex_unlock(&_deferred_lock); 
		lua_pushboolean(L, true); 
		return 1; 
	}

	TRACE("Inserting new deferred command [%s] with timeout %d\n", cmd, timeout); 
	cm = calloc(1, sizeof(struct deferred_command)); 
	cm->command = calloc(1, strlen(cmd) + 1); 
	timespec_from_now_us(&cm->ts_run, timeout); 
	strcpy(cm->command, cmd); 
	cm->avl.key = cm->command; 
	
	avl_insert(&_deferred_commands, &cm->avl); 
	pthread_cond_signal(&_deferred_cond); 
	pthread_mutex_unlock(&_deferred_lock); 

	lua_pushboolean(L, true); 
	return 1; 
}

// -- DEFERRED SHELL
// ++ NAMED LOCKS IMPLEMENTATION
static struct avl_tree _locks; 
static pthread_mutex_t _locks_lock = PTHREAD_MUTEX_INITIALIZER; 

struct lock_node {
	struct avl_node avl; 
	char *name; 
	pthread_mutex_t lock; 
}; 

static void __attribute__((constructor)) _locks_init(void){
	avl_init(&_locks, avl_strcmp, false, NULL);
}

static int l_core_lock(lua_State *L){
	const char *name = luaL_checkstring(L, 1); 
	if(!name){
		lua_pushboolean(L, false); 
		return 1; 
	}
	pthread_mutex_lock(&_locks_lock); 
	struct lock_node *node = avl_find_element(&_locks, name, node, avl);
	if(!node){
		node = calloc(1, sizeof(struct lock_node)); 
		node->name = calloc(1, strlen(name) + 1); 
		strcpy(node->name, name); 
		node->avl.key = node->name; 
		pthread_mutex_init(&node->lock, NULL); 
		avl_insert(&_locks, &node->avl); 
	} 

	TRACE("lock: locking named lock (%s)\n", node->name); 
	pthread_mutex_lock(&node->lock); 

	pthread_mutex_unlock(&_locks_lock); 

	lua_pushboolean(L, true); 
	return 1; 
}

static int l_core_unlock(lua_State *L){
	const char *name = luaL_checkstring(L, 1); 
	if(!name){
		lua_pushboolean(L, false); 
		return 1; 
	}
	pthread_mutex_lock(&_locks_lock); 
	struct lock_node *node = avl_find_element(&_locks, name, node, avl);
	if(!node){
		pthread_mutex_unlock(&_locks_lock); 
		lua_pushboolean(L, false); 
		return 1; 
	}

	TRACE("lock: unlocking named lock (%s)\n", node->name); 
	pthread_mutex_unlock(&node->lock); 

	// free the node
	avl_delete(&_locks, &node->avl); 
	pthread_mutex_destroy(&node->lock);  
	free(node->name); 
	free(node); 

	pthread_mutex_unlock(&_locks_lock); 

	lua_pushboolean(L, true); 
	return 1; 
}

// -- NAMED LOCKS
static int l_core_b64e(lua_State *L){
	const char *cmd = luaL_checkstring(L, 1); 
	if(!cmd) {
		lua_pushboolean(L, false); 
		return 1; 
	}
	int size = strlen(cmd) * 2 + 1; 
	char *b64string = malloc(size); 
	memset(b64string, 0, size); 
	int n = base64_encode(cmd, b64string, size); 
	b64string[n] = 0; 
	TRACE("b64: %s %s %d\n", cmd, b64string, n); 
	lua_pushstring(L, b64string); 
	free(b64string); 
	return 1; 
}

//! used by lua scripts to send user signal to current process
static int l_core_interrupt(lua_State *L){
	int ret = luaL_checkint(L, 1); 
	raise(SIGUSR1); 
	return 0; 
}

void orange_lua_publish_session_api(lua_State *L){
	lua_newtable(L); 
	lua_pushstring(L, "access"); lua_pushcfunction(L, l_session_access); lua_settable(L, -3); 
	lua_pushstring(L, "get"); lua_pushcfunction(L, l_session_get); lua_settable(L, -3); 
	lua_setglobal(L, "SESSION"); 
}

void orange_lua_publish_core_api(lua_State *L){
	lua_newtable(L); 
	lua_pushstring(L, "forkshell"); lua_pushcfunction(L, l_core_fork_shell); lua_settable(L, -3); 
	lua_pushstring(L, "deferredshell"); lua_pushcfunction(L, l_core_deferred_shell); lua_settable(L, -3); 
	lua_pushstring(L, "lock"); lua_pushcfunction(L, l_core_lock); lua_settable(L, -3); 
	lua_pushstring(L, "unlock"); lua_pushcfunction(L, l_core_unlock); lua_settable(L, -3); 
	lua_pushstring(L, "b64_encode"); lua_pushcfunction(L, l_core_b64e); lua_settable(L, -3); 
	lua_pushstring(L, "__interrupt"); lua_pushcfunction(L, l_core_interrupt); lua_settable(L, -3); 
	lua_setglobal(L, "CORE"); 
}

void orange_lua_set_session(lua_State *L, struct orange_session *self){
	lua_getglobal(L, "SESSION"); 
	luaL_checktype(L, -1, LUA_TTABLE); 
	lua_pushstring(L, "_self"); lua_pushlightuserdata(L, self); lua_settable(L, -3); 
	lua_pop(L, 1); 
}

