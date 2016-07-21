#pragma once 

#include <libutype/avl.h>

struct orange_user_acl {
	struct avl_node avl; 
}; 

struct orange_user {
	struct avl_node avl; 
	char *username; 
	char *pwhash; 
	struct avl_tree acls; 
}; 

struct orange_user *orange_user_new(const char *username); 
void orange_user_delete(struct orange_user **self); 

void orange_user_add_acl(struct orange_user *self, const char *acl); 
void orange_user_set_pw_hash(struct orange_user *self, const char *pwhash); 

#define orange_user_for_each_acl(user, var) avl_for_each_element(&(user)->acls, var, avl)
