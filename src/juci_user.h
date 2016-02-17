#pragma once 

#include <libutype/avl.h>

struct juci_user {
	struct avl_node avl; 
	char *username; 
	char *pwhash; 
}; 

struct juci_user *juci_user_new(const char *username); 
void juci_user_delete(struct juci_user **self); 

void juci_user_set_pw_hash(struct juci_user *self, const char *pwhash); 
