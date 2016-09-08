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

#include <stdio.h>
#include <unistd.h>
#include <dirent.h>
#include <glob.h>

#include <fcntl.h>

#include <shadow.h>
#include <sys/types.h>
#include <pwd.h>
#include <crypt.h>

#include <pthread.h>

#include <blobpack/blobpack.h>

#if HAVE_UCI_H
#include <uci.h>
#endif

#include "orange.h"
#include "orange_luaobject.h"
#include "orange_lua.h"
#include "orange_user.h"

#include "sha1.h"

#define JUCI_ACL_DIR_PATH "/usr/lib/orange/acl/"

int orange_debug_level = 0; 

static int _orange_load_plugins(struct orange *self, const char *path, const char *base_path){
    int rv = 0; 
    if(!base_path) base_path = path; 
    DIR *dir = opendir(path); 
    if(!dir){
        ERROR("could not open directory %s\n", path); 
        return -ENOENT; 
    }
    struct dirent *ent = 0; 
    char fname[255]; 
    while((ent = readdir(dir))){
        if(strcmp(ent->d_name, ".") == 0 || strcmp(ent->d_name, "..") == 0) continue; 

        snprintf(fname, sizeof(fname), "%s/%s", path, ent->d_name); 
        
        if(ent->d_type == DT_DIR) {
            rv |= _orange_load_plugins(self, fname, base_path);  
        } else  if(ent->d_type == DT_REG || ent->d_type == DT_LNK){
			// TODO: is there a better way to get filename without extension? 
			char *ext = strrchr(fname, '.');  
			if(!ext) continue;

			char *name = fname + strlen(base_path); 
			int len = strlen(name); 
			char *objname = alloca( len + 1 ); 
			assert(objname); 
			strncpy(objname, name, len - strlen(ext)); 

			if(strcmp(ext, ".lua") != 0) continue; 
			objname[len - strlen(ext)] = 0; 

			struct avl_node *node = avl_find(&self->objects, objname); 
			if(node) {
				ERROR("ERR: trying to load plugin for object that already exists in the directory!\n"); 
				continue; 
			}

			INFO("loading plugin %s of %s at base %s\n", objname, fname, base_path); 

			struct orange_luaobject *obj = orange_luaobject_new(objname); 
			if(orange_luaobject_load(obj, fname) != 0){
				ERROR("ERR: could not load plugin %s\n", fname); 
				orange_luaobject_delete(&obj); 
				continue; 
			}
			
			// add to directory
			avl_insert(&self->objects, &obj->avl); 

			orange_lua_publish_json_api(obj->lua); 
			orange_lua_publish_file_api(obj->lua); 
			orange_lua_publish_session_api(obj->lua); 
			orange_lua_publish_core_api(obj->lua); 
		}
    }
    closedir(dir); 
    return rv; 
}

// taken from rpcd source code (session.c)
#ifdef HAVE_UCI_H
static void _orange_user_load_acls(struct orange_user *self, struct uci_section *s){
	struct uci_option *o;
	struct uci_element *e, *l;

	uci_foreach_element(&s->options, e){
		o = uci_to_option(e);

		if (o->type != UCI_TYPE_LIST)
			continue;

		if (strcmp(o->e.name, "acls"))
			continue;
		
		uci_foreach_element(&o->v.list, l) {
			orange_user_add_acl(self, l->name); 
		}
	}
}
#endif
static bool _orange_load_users(struct orange *self){
	// TODO: we need to fix this 
#ifdef HAVE_UCI_H
	static const char *config_name = "orange"; 
	struct uci_package *p = NULL;
	struct uci_section *s;
	struct uci_element *e;
	struct uci_ptr ptr = { .package = config_name };
	struct uci_context *uci = uci_alloc_context(); 

	uci_load(uci, ptr.package, &p);

	if (!p) {
		INFO("Could not find config file /etc/config/%s. Not loading any users!\n", config_name); 
		return false;
	}

	INFO("loading users from config /etc/config/%s\n", config_name); 

	uci_foreach_element(&p->sections, e){
		s = uci_to_section(e);

		if (strcmp(s->type, "login"))
			continue;
		
		struct orange_user *user = orange_user_new(s->e.name); 
		
		TRACE("added user '%s'\n", s->e.name); 

		_orange_user_load_acls(user, s); 

		avl_insert(&self->users, &user->avl); 
	}

	uci_free_context(uci); 
#endif
	return true; 
}

static char *_load_file(const char *path){
	int fd = open(path, O_RDONLY); 
	if(fd == -1) return NULL; 
	int filesize = lseek(fd, 0, SEEK_END); 
	lseek(fd, 0, SEEK_SET); 
	char *text = calloc(1, filesize + 1); 
	assert(text); 
	int ret = read(fd, text, filesize); 
	close(fd); 
	if(ret != filesize) { free(text); return NULL; }
	return text; 
}

static int _orange_load_passwords(struct orange *self, const char *pwfile){
	DEBUG("loading passwords from %s\n", pwfile); 
	char *passwords = _load_file(pwfile); 
	if(!passwords) return -EACCES; 
	char *cur = passwords; 
	int line = 0; 
	while(1){	
		char *nl = strchr(cur, '\n'); 
		if(nl) *nl = 0; 
		char *user = strtok(cur, " "); 
		char *hash = strtok(NULL, " "); 
		if(user && hash){
			struct avl_node *node = avl_find(&self->users, user); 
			if(node){
				struct orange_user *u = container_of(node, struct orange_user, avl); 
				if(u){
					orange_user_set_pw_hash(u, hash); 
					DEBUG("updated existing user %s\n", user); 
				}
			} 
		} else {
			ERROR("Could not load user password on line %d: expected format <user> <hash>!\n", line); 
		}
		if(!nl) break; 
		cur = nl; *cur = '\n'; // restore the new line
		// skip until begining of next line
		while(*cur != '\n' && *cur) cur++; 
		while(*cur == '\n') cur++; 
		if(*cur == 0) break; 
		line++; 
	}
	free(passwords); 
	return 0; 
}

static struct orange_session* _find_session(struct orange *self, const char *sid){
	if(!sid || strlen(sid) == 0) return NULL; 
	struct avl_node *id = avl_find(&self->sessions, sid); 	
	if(!id) return NULL;  
	return container_of(id, struct orange_session, avl); 
}

static bool _try_auth(const char *sha1hash, const char *challenge, const char *response){
	if(!sha1hash) return false; 

	DEBUG("trying to authenticate hash %s using challenge %s and response %s\n", (sha1hash)?sha1hash:"(null)", (challenge)?challenge:"(null)", response); 
	
	unsigned char binhash[SHA1_BLOCK_SIZE+1] = {0}; 
	SHA1_CTX ctx; 
	sha1_init(&ctx); 
	if(challenge) sha1_update(&ctx, (const unsigned char*)challenge, strlen(challenge)); 
	sha1_update(&ctx, (const unsigned char*)sha1hash, strlen(sha1hash)); 
	sha1_final(&ctx, binhash); 
	char hash[SHA1_BLOCK_SIZE*2+1] = {0}; 
	for(int c = 0; c < SHA1_BLOCK_SIZE; c++) sprintf(hash + (c * 2), "%02x", binhash[c]); 

	DEBUG("authenticating against digest %s\n", hash); 
	return !strncmp((const char*)hash, response, SHA1_BLOCK_SIZE*2); 
}

static int _load_session_acls(struct orange_session *ses, const char *dir, const char *pat){
	glob_t glob_result;
	char path[255]; 
	if(!dir || !strlen(dir)) dir = getenv("JUCI_ACL_DIR_PATH"); 
	if(!dir) dir = JUCI_ACL_DIR_PATH; 
	DEBUG("loading acls from %s/%s.acl\n", dir, pat); 
	snprintf(path, sizeof(path), "%s/%s.acl", dir, pat); 
	glob(path, 0, NULL, &glob_result);
	for(unsigned int i=0;i<glob_result.gl_pathc;++i){
		char *text = _load_file(glob_result.gl_pathv[i]); 
		if(!text) continue; 
		char *cur = text; 	
		int line = 1; 
		while(true){
			char *nl = strchr(cur, '\n'); 
			if(nl) *nl = 0; 
			char *scope = strtok(cur, " "); 
			char *object = strtok(NULL, " "); 
			char *method = strtok(NULL, " "); 
			char *perm = strtok(NULL, " "); 
			if(scope && object && method && perm){
				if(scope[0] == '!'){
					DEBUG("revoking session acl '%s %s %s %s'\n", scope + 1, object, method, perm); 
					orange_session_revoke(ses, scope + 1, object, method, perm); 
				} else {
					DEBUG("granting session acl '%s %s %s %s'\n", scope, object, method, perm); 
					orange_session_grant(ses, scope, object, method, perm); 
				} 
			} else {
				ERROR("parse error on line %d of %s: expected 4 fields separated by spaces!\n", line, glob_result.gl_pathv[i]); 	
			}
			if(!nl) break; // if this was the last line then we break 
			cur = nl; *cur = '\n'; // restore newline 
			while(*cur != '\n' && *cur != 0) cur++; 
			while(*cur == '\n') cur++; 
			if(*cur == 0) break; 
			line++; 
		}
		free(text); 
	}
	globfree(&glob_result);
	return 0; 
}

void orange_add_user(struct orange *self, struct orange_user **user){
	pthread_mutex_lock(&self->lock); 
	avl_insert(&self->users, &(*user)->avl); 
	*user = NULL; 
	pthread_mutex_unlock(&self->lock); 
}

struct orange* orange_new(const char *plugin_path, const char *pwfile, const char *acl_path ){
	struct orange *self = calloc(1, sizeof(struct orange)); 
	assert(self); 
	avl_init(&self->objects, avl_strcmp, false, NULL); 
	avl_init(&self->sessions, avl_strcmp, false, NULL); 
	avl_init(&self->users, avl_strcmp, false, NULL); 
	
	_orange_load_users(self); 

	self->plugin_path = strdup(plugin_path); 
	self->pwfile = strdup(pwfile); 
	self->acl_path = strdup(acl_path); 

	pthread_mutex_init(&self->lock, NULL); 

	if(_orange_load_passwords(self, self->pwfile) != 0){
		ERROR("could not load password file from %s\n", pwfile); 
	}

	_orange_load_plugins(self, self->plugin_path, NULL); 

	return self; 
}

void orange_delete(struct orange **_self){
	struct orange *self = *_self; 
	struct orange_luaobject *obj, *nobj;
    struct orange_session *ses, *nses;
    struct orange_user *user, *nuser;

	avl_remove_all_elements(&self->objects, obj, avl, nobj)
		orange_luaobject_delete(&obj); 

    avl_remove_all_elements(&self->sessions, ses, avl, nses)
		orange_session_delete(&ses); 

    avl_remove_all_elements(&self->users, user, avl, nuser)
		orange_user_delete(&user); 
	
	free(self->pwfile); 
	free(self->plugin_path); 
	free(self->acl_path); 

	pthread_mutex_destroy(&self->lock); 

	free(self); 
	_self = NULL; 
}

bool orange_session_is_valid(struct orange *self, const char *sid){
	pthread_mutex_lock(&self->lock); 
	struct orange_session *ses = _find_session(self, sid); 
	bool ret = !!ses; 
	pthread_mutex_unlock(&self->lock); 
	return ret; 
}
/*
struct orange_session *orange_find_session(struct orange *self, const char *sid){ 
	return _find_session(self, sid); 
}
*/
int orange_login(struct orange *self, const char *username, const char *challenge, const char *response, struct orange_sid *sid){
	// always reset the output value
	memset(sid, 0, sizeof(struct orange_sid)); 

	pthread_mutex_lock(&self->lock); 

	struct avl_node *node = avl_find(&self->users, username); 
	if(!node){
		DEBUG("user %s not found!\n", username);
		pthread_mutex_unlock(&self->lock); 
		return -EINVAL; 
	}
	struct orange_user *user = container_of(node, struct orange_user, avl); 

	// update user hashes (TODO: perhaps this should be loaded into secure memory somehow and discarded after login?)
	_orange_load_passwords(self, self->pwfile); 

	if(_try_auth(user->pwhash, challenge, response)){
		struct orange_session *ses = orange_session_new(user); 	
		struct orange_user_acl *acl; 
		orange_user_for_each_acl(user, acl){
			DEBUG("loading session acl from user profile: %s\n", (const char*)acl->avl.key); 
			_load_session_acls(ses, self->acl_path, acl->avl.key); 
		}
		struct blob buf; 
		blob_init(&buf, 0, 0); 
		orange_session_to_blob(ses, &buf); 
		//blob_dump_json(&buf); 
		blob_free(&buf); 
		if(avl_insert(&self->sessions, &ses->avl) != 0){
			DEBUG("could not insert session!\n");
			orange_session_delete(&ses); 
			pthread_mutex_unlock(&self->lock); 
			return -EINVAL; 
		}
		memcpy(sid, &ses->sid, sizeof(struct orange_sid)); 
		pthread_mutex_unlock(&self->lock); 
		return 0; 
	} else {
		DEBUG("login failed for %s!\n", username); 
	}
	pthread_mutex_unlock(&self->lock); 
	return -EACCES; 
}

int orange_login_plaintext(struct orange *self, const char *username, const char *password, struct orange_sid *sid){
	// NOTE: no locking required. Done by orange_login()
	unsigned char binhash[SHA1_BLOCK_SIZE+1] = {0}; 
	SHA1_CTX ctx; 
	sha1_init(&ctx); 
	sha1_update(&ctx, (const unsigned char*)password, strlen(password)); 
	sha1_final(&ctx, binhash); 
	char hash[SHA1_BLOCK_SIZE*2+1] = {0}; 
	for(int c = 0; c < SHA1_BLOCK_SIZE; c++) sprintf(hash + (c * 2), "%02x", binhash[c]); 
	
	// hash again to simulate what would happen on the client
	sha1_init(&ctx); 
	sha1_update(&ctx, (const unsigned char*)hash, SHA1_BLOCK_SIZE * 2); 
	sha1_final(&ctx, binhash); 
	for(int c = 0; c < SHA1_BLOCK_SIZE; c++) sprintf(hash + (c * 2), "%02x", binhash[c]); 

	return orange_login(self, username, NULL, hash, sid); 
}

int orange_logout(struct orange *self, const char *sid){
	pthread_mutex_lock(&self->lock); 
	struct avl_node *node = avl_find(&self->sessions, sid); 
	if(!node) {
		pthread_mutex_unlock(&self->lock); 
		return -EINVAL; 
	}
	struct orange_session *ses = container_of(node, struct orange_session, avl); 
	avl_delete(&self->sessions, node); 
	orange_session_delete(&ses); 
	pthread_mutex_unlock(&self->lock); 
	return 0; 
}

int orange_call(struct orange *self, const char *sid, const char *object, const char *method, struct blob_field *args, struct blob *out){
	pthread_mutex_lock(&self->lock); 

	struct avl_node *avl = avl_find(&self->objects, object); 
	if(!avl) {
		ERROR("object not found: %s\n", object); 
		pthread_mutex_unlock(&self->lock); 
		return -ENOENT; 
	}
	
	struct orange_session *ses = _find_session(self, sid); 
	if(ses) {
		DEBUG("found session for request: %s\n", sid); 
	} else {
		DEBUG("could not find session for request!\n"); 
		pthread_mutex_unlock(&self->lock); 
		return -EACCES; 
	}

	if(	!orange_session_access(ses, "rpc", object, method, "x") && 
		!orange_session_access(ses, "ubus", object, method, "x") // deprecated
	){
		ERROR("user %s does not have permission to execute rpc call: %s %s\n", ses->user->username, object, method); 
		pthread_mutex_unlock(&self->lock); 
		return -EACCES; 
	}
	
	size_t fnsize = strlen(self->plugin_path) + strlen(object) + 6; 
	char *fname = alloca(fnsize); 
	snprintf(fname, fnsize, "%s/%s.lua", self->plugin_path, object); 

	// From here onwards we do not use anything inside self!
	// so we enable concurrency and run the actual backend script
	pthread_mutex_unlock(&self->lock); 
	
	INFO("loading lua object %s from %s...\n", object, fname); 

	struct orange_luaobject *obj = orange_luaobject_new(object); 
	if(orange_luaobject_load(obj, fname) != 0){
		ERROR("ERR: could not load plugin %s\n", fname); 
		orange_luaobject_delete(&obj); 
		return -ENOENT; 
	}

	// export server api to the lua object
	orange_lua_publish_json_api(obj->lua); 
	orange_lua_publish_file_api(obj->lua); 
	orange_lua_publish_session_api(obj->lua); 
	orange_lua_publish_core_api(obj->lua); 
	
	int ret = orange_luaobject_call(obj, ses, method, args, out); 

	orange_luaobject_delete(&obj); 

	return ret; 
}

int orange_list(struct orange *self, const char *sid, const char *path, struct blob *out){
	pthread_mutex_lock(&self->lock); 
	struct orange_luaobject *entry; 
	blob_offset_t t = blob_open_table(out); 
	avl_for_each_element(&self->objects, entry, avl){
		blob_put_string(out, (const char*)entry->avl.key); 
		blob_put_attr(out, blob_field_first_child(blob_head(&entry->signature))); 
	}
	blob_close_table(out, t); 
	pthread_mutex_unlock(&self->lock); 
	return 0; 
}


