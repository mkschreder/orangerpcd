#pragma once

// 1 extra byte for trailing zero
typedef char juci_sid_t[32 + 1]; 

#include <libutype/avl.h>
struct juci_session {
	struct avl_node avl; 
	juci_sid_t sid; 
	struct avl_tree data; 
	struct avl_tree acl_scopes; 
	
	struct juci_user *user; 
}; 

struct juci_session *juci_session_new(struct juci_user *user); 
void juci_session_delete(struct juci_session **self); 
bool juci_session_grant(struct juci_session *self, const char *scope, const char *object, const char *method); 
bool juci_session_revoke(struct juci_session *self, const char *scope, const char *object, const char *method); 
bool juci_session_can_access(struct juci_session *self, const char *scope, const char *object, const char *method); 
