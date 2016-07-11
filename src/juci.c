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

#include <blobpack/blobpack.h>

#include <uci.h>

#include "juci.h"
#include "juci_luaobject.h"
#include "juci_lua.h"
#include "juci_user.h"

#include "sha1.h"

#define JUCI_ACL_DIR_PATH "/usr/lib/juci/acl/"

int juci_debug_level = 0; 

int juci_load_passwords(struct juci *self, const char *pwfile); 
int juci_load_plugins(struct juci *self, const char *path, const char *base_path){
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
            rv |= juci_load_plugins(self, fname, base_path);  
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
			INFO("loading plugin %s of %s at base %s\n", objname, fname, base_path); 
			struct juci_luaobject *obj = juci_luaobject_new(objname); 
			if(juci_luaobject_load(obj, fname) != 0 || avl_insert(&self->objects, &obj->avl) != 0){
				ERROR("ERR: could not load plugin %s\n", fname); 
				juci_luaobject_delete(&obj); 
				continue; 
			}
			juci_lua_publish_json_api(obj->lua); 
			juci_lua_publish_file_api(obj->lua); 
			juci_lua_publish_session_api(obj->lua); 
		}
    }
    closedir(dir); 
    return rv; 
}

// taken from rpcd source code (session.c)

static void _juci_user_load_acls(struct juci_user *self, struct uci_section *s){
	struct uci_option *o;
	struct uci_element *e, *l;

	uci_foreach_element(&s->options, e){
		o = uci_to_option(e);

		if (o->type != UCI_TYPE_LIST)
			continue;

		if (strcmp(o->e.name, "acls"))
			continue;
		
		uci_foreach_element(&o->v.list, l) {
			juci_user_add_acl(self, l->name); 
		}
	}
}

static bool _juci_load_users(struct juci *self){
	struct uci_package *p = NULL;
	struct uci_section *s;
	struct uci_element *e;
	struct uci_ptr ptr = { .package = "jucid" };
	struct uci_context *uci = uci_alloc_context(); 

	uci_load(uci, ptr.package, &p);

	if (!p)
		return false;

	uci_foreach_element(&p->sections, e)
	{
		s = uci_to_section(e);

		if (strcmp(s->type, "login"))
			continue;
		
		struct juci_user *user = juci_user_new(s->e.name); 

		_juci_user_load_acls(user, s); 
		
		TRACE("JUCI: loaded user config for user '%s'\n", s->e.name); 

		avl_insert(&self->users, &user->avl); 
	}

	uci_free_context(uci); 

	return true; 
}

struct juci* juci_new(const char *plugin_path, const char *pwfile){
	struct juci *self = calloc(1, sizeof(struct juci)); 
	assert(self); 
	avl_init(&self->objects, avl_strcmp, false, NULL); 
	avl_init(&self->sessions, avl_strcmp, false, NULL); 
	avl_init(&self->users, avl_strcmp, false, NULL); 

	// TODO: load users from config file
	_juci_load_users(self); 
	/*
	struct juci_user *admin = juci_user_new("admin"); 
	juci_user_add_acl(admin, "juci*"); 
	juci_user_add_acl(admin, "user-admin"); 
	avl_insert(&self->users, &admin->avl); 

	struct juci_user *support = juci_user_new("support"); 
	juci_user_add_acl(support, "juci*"); 
	juci_user_add_acl(support, "user-support"); 
	avl_insert(&self->users, &support->avl); 
*/	
	self->plugin_path = strdup(plugin_path); 
	self->pwfile = strdup(pwfile); 

	if(juci_load_passwords(self, self->pwfile) != 0){
		ERROR("could not load password file from %s\n", pwfile); 
	}
	juci_load_plugins(self, self->plugin_path, NULL); 
	

	return self; 
}

void juci_delete(struct juci **_self){
	struct juci *self = *_self; 
	struct juci_luaobject *obj, *nobj;
    struct juci_session *ses, *nses;
    struct juci_user *user, *nuser;

	avl_remove_all_elements(&self->objects, obj, avl, nobj)
		juci_luaobject_delete(&obj); 

    avl_remove_all_elements(&self->sessions, ses, avl, nses)
		juci_session_delete(&ses); 

    avl_remove_all_elements(&self->users, user, avl, nuser)
		juci_user_delete(&user); 
	
	free(self->pwfile); 
	free(self->plugin_path); 

	free(self); 
	_self = NULL; 
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

int juci_load_passwords(struct juci *self, const char *pwfile){
	DEBUG("loading passwords from %s\n", pwfile); 
	char *passwords = _load_file(pwfile); 
	if(!passwords) return -EACCES; 
	char *cur = passwords; 
	char user[32], hash[64]; 
	while(sscanf(cur, "%s %s", user, hash) == 2){	
		struct avl_node *node = avl_find(&self->users, user); 
		if(node){
			struct juci_user *u = container_of(node, struct juci_user, avl); 
			if(u){
				juci_user_set_pw_hash(u, hash); 
				DEBUG("updated existing user %s\n", user); 
			}
		} else {
			// create new user
			struct juci_user *u = juci_user_new(user); 
			avl_insert(&self->users, &u->avl); 
			DEBUG("loaded new user %s\n", user); 
		}

		while(*cur != '\n' && *cur) cur++; 
		while(*cur == '\n') cur++; 
		if(*cur == 0) break; 
	}
	free(passwords); 
	return 0; 
}

static struct juci_session* _find_session(struct juci *self, const char *sid){
	if(!sid || strlen(sid) == 0) return NULL; 
	struct avl_node *id = avl_find(&self->sessions, sid); 	
	if(!id) return NULL;  
	return container_of(id, struct juci_session, avl); 
}

static bool _try_auth(const char *sha1hash, const char *challenge, const char *response){
	DEBUG("trying to authenticate hash %s using challenge %s and response %s\n", sha1hash, challenge, response); 
	if(!sha1hash) return false; 
	
	unsigned char binhash[SHA1_BLOCK_SIZE+1] = {0}; 
	SHA1_CTX ctx; 
	sha1_init(&ctx); 
	sha1_update(&ctx, (const unsigned char*)challenge, strlen(challenge)); 
	sha1_update(&ctx, (const unsigned char*)sha1hash, strlen(sha1hash)); 
	sha1_final(&ctx, binhash); 
	char hash[64] = {0}; 
	for(int c = 0; c < SHA1_BLOCK_SIZE; c++) sprintf(hash + (c * 2), "%02x", binhash[c]); 

	DEBUG("authenticating against digest %s\n", hash); 
	return !strcmp((const char*)hash, response); 
}

int _load_session_acls(struct juci_session *ses, const char *pat){
	glob_t glob_result;
	char path[255]; 
	char *dir = getenv("JUCI_ACL_DIR_PATH"); 
	if(!dir) dir = JUCI_ACL_DIR_PATH; 
	DEBUG("loading acls from %s/%s.acl\n", dir, pat); 
	snprintf(path, sizeof(path), "%s/%s.acl", dir, pat); 
	glob(path, GLOB_TILDE, NULL, &glob_result);
	for(unsigned int i=0;i<glob_result.gl_pathc;++i){
		char *text = _load_file(glob_result.gl_pathv[i]); 
		char *cur = text; 	
		char scope[255], object[255], method[255], perm[32]; 
		int line = 1; 
		while(true){	
			int ret = sscanf(cur, "%s %s %s %s", scope, object, method, perm); 
			if(ret == 4 && scope[0] == '!'){
				DEBUG("revoking session acl '%s %s %s %s'\n", scope + 1, object, method, perm); 
				juci_session_revoke(ses, scope + 1, object, method, perm); 
			} else if(ret == 4){
				DEBUG("granting session acl '%s %s %s %s'\n", scope, object, method, perm); 
				juci_session_grant(ses, scope, object, method, perm); 
			} else {
				ERROR("parse error on line %d of %s, scanned %d fields\n", line, glob_result.gl_pathv[i], ret); 	
			} 
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

struct juci_session *juci_find_session(struct juci *self, const char *sid){ 
	return _find_session(self, sid); 
}

int juci_login(struct juci *self, const char *username, const char *challenge, const char *response, const char **new_sid){
	struct avl_node *node = avl_find(&self->users, username); 
	if(!node){
		DEBUG("user %s not found!\n", username);
		return -EINVAL; 
	}
	struct juci_user *user = container_of(node, struct juci_user, avl); 
	// update user hashes (TODO: perhaps this should be loaded into secure memory somehow and discarded after login?)
	juci_load_passwords(self, self->pwfile); 

	if(_try_auth(user->pwhash, challenge, response)){
		struct juci_session *ses = juci_session_new(user); 	
		struct juci_user_acl *acl; 
		juci_user_for_each_acl(user, acl){
			_load_session_acls(ses, acl->avl.key); 
		}
		struct blob buf; 
		blob_init(&buf, 0, 0); 
		juci_session_to_blob(ses, &buf); 
		//blob_dump_json(&buf); 
		blob_free(&buf); 
		if(avl_insert(&self->sessions, &ses->avl) != 0){
			DEBUG("could not insert session!\n");
			juci_session_delete(&ses); 
			return -EINVAL; 
		}
		*new_sid = ses->sid; 
		return 0; 
	} else {
		DEBUG("login failed for %s!\n", username); 
	}
	return -EACCES; 
}

int juci_logout(struct juci *self, const char *sid){
	struct avl_node *node = avl_find(&self->sessions, sid); 
	if(!node) return -EINVAL; 
	struct juci_session *ses = container_of(node, struct juci_session, avl); 
	avl_delete(&self->sessions, node); 
	juci_session_delete(&ses); 
	return 0; 
}

int juci_call(struct juci *self, const char *sid, const char *object, const char *method, struct blob_field *args, struct blob *out){
	struct avl_node *avl = avl_find(&self->objects, object); 
	if(!avl) {
		ERROR("object not found: %s\n", object); 
		return -ENOENT; 
	}
	struct juci_luaobject *obj = container_of(avl, struct juci_luaobject, avl); 
	self->current_session = _find_session(self, sid); 
	if(self->current_session) {
		DEBUG("found session for request: %s\n", sid); 
	} else {
		DEBUG("could not find session for request!\n"); 
		return -EACCES; 
	}
	if(!juci_session_access(self->current_session, "ubus", object, method, "x")){
		ERROR("user %s does not have permission to execute rpc call: %s %s\n", self->current_session->user->username, object, method); 
		return -EACCES; 
	}
	return juci_luaobject_call(obj, self->current_session, method, args, out); 
}

int juci_list(struct juci *self, const char *sid, const char *path, struct blob *out){
	struct juci_luaobject *entry; 
	blob_offset_t t = blob_open_table(out); 
	avl_for_each_element(&self->objects, entry, avl){
		blob_put_string(out, (char*)entry->avl.key); 
		blob_put_attr(out, blob_field_first_child(blob_head(&entry->signature))); 
	}
	blob_close_table(out, t); 
	return 0; 
}


