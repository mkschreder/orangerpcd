#pragma once

typedef char juci_sid_t[32]; 

#include <libutype/avl.h>
struct juci_session {
	struct avl_node avl; // for organizing sessions
	juci_sid_t sid; 
	struct avl_tree data; 
	struct avl_tree acls; 
}; 

struct juci_session_data {
	struct avl_node avl;
	struct blob_field *attr;
};

struct juci_session_acl_scope {
	struct avl_node avl;
	struct avl_tree acls;
};

struct juci_session_acl {
	struct avl_node avl;
	const char *object;
	const char *function;
	int sort_len;
};

struct juci_session *juci_session_new(); 
