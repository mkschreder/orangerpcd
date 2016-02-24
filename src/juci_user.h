#pragma once 

#include <libutype/avl.h>

struct juci_user_acl {
	struct avl_node avl; 
}; 

struct juci_user {
	struct avl_node avl; 
	char *username; 
	char *pwhash; 
	struct avl_tree acls; 
}; 

struct juci_user *juci_user_new(const char *username); 
void juci_user_delete(struct juci_user **self); 

void juci_user_add_acl(struct juci_user *self, const char *acl); 
void juci_user_set_pw_hash(struct juci_user *self, const char *pwhash); 

#define juci_user_for_each_acl(user, var) avl_for_each_element(&(user)->acls, var, avl)
