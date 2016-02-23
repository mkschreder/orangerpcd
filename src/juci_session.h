#pragma once

// 1 extra byte for trailing zero
typedef char juci_sid_t[32 + 1]; 

#include <libutype/avl.h>
#include "juci_user.h"

struct juci_session {
	struct avl_node avl; 
	juci_sid_t sid; 
	struct avl_tree data; 
	struct avl_tree acl_scopes; 
	
	struct juci_user *user; 
}; 

struct juci_session *juci_session_new(struct juci_user *user); 
void juci_session_delete(struct juci_session **self); 
int juci_session_grant(struct juci_session *self, const char *scope, const char *object, const char *method, const char *perm); 
int juci_session_revoke(struct juci_session *self, const char *scope, const char *object, const char *method, const char *perm); 
bool juci_session_access(struct juci_session *ses, const char *scope, const char *obj, const char *fun, const char *perm); 
