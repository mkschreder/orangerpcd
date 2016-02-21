#include <stdlib.h>
#include <string.h>

#include <blobpack/blobpack.h>
#include "juci_user.h"

struct juci_user_cap {
	struct avl_node avl; 
	char *name; 
}; 

struct juci_user_app {
	struct avl_node avl; 
	char *name; 
}; 

struct juci_user *juci_user_new(const char *username){
	struct juci_user *self = calloc(1, sizeof(struct juci_user)); 
	assert(self); 
	self->username = strdup(username); 
	self->avl.key = self->username; 
	return self; 
}

void juci_user_delete(struct juci_user **_self){
	struct juci_user *self = *_self; 	
	if(self->pwhash) free(self->pwhash); 
	free(self->username); 
	free(self); 
	*_self = NULL; 
}

void juci_user_set_pw_hash(struct juci_user *self, const char *pwhash){
	if(self->pwhash) free(self->pwhash); 
	self->pwhash = strdup(pwhash); 
}

void juci_user_add_application(struct juci_user *self, const char *app){
	
}

void juci_user_add_capability(struct juci_user *self, const char *cap){

}

int juci_user_from_blob_table(struct juci_user *self, struct blob_field *field){
	struct blob_field *key, *value; 	
	blob_field_for_each_kv(field, key, value){
		const char *k = blob_field_get_string(key); 
		const char *v = blob_field_get_string(value); 
		if(strcmp(k, "password") == 0){
			juci_user_set_pw_hash(self, v); 
		} else if(strcmp(k, "apps") == 0){
			struct blob_field *ch = 0; 
			blob_field_for_each_child(value, ch){
				//const char *appname = blob_field_get_string(ch); 
				//juci_user_add_app_access(self, appname, true); 
			}
		} else if(strcmp(k, "caps") == 0){
			struct blob_field *ch = 0; 
			blob_field_for_each_child(value, ch){
				//const char *capname = blob_field_get_string(ch); 
				//juci_user_add_capability(self, capname); 
			}
		}
	}
	return 0; 
}

