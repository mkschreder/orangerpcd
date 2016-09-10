#pragma once

#include <libutype/avl.h>
#include "orange_user.h"

struct orange_sid {
	// 1 extra byte for trailing zero
	char hash[32 + 1]; 
}; 

struct orange_session {
	struct avl_node avl; 
	struct orange_sid sid; 
	struct avl_tree data; 
	struct avl_tree acl_scopes; 

	pthread_mutex_t lock; 
	struct orange_user *user; 
}; 

struct orange_session *orange_session_new(struct orange_user *user); 
void orange_session_delete(struct orange_session **self); 
int orange_session_grant(struct orange_session *self, const char *scope, const char *object, const char *method, const char *perm); 
int orange_session_revoke(struct orange_session *self, const char *scope, const char *object, const char *method, const char *perm); 
bool orange_session_access(struct orange_session *ses, const char *scope, const char *obj, const char *fun, const char *perm); 

void orange_session_to_blob(struct orange_session *self, struct blob *buf); 
