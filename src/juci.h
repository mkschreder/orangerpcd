#pragma once 

#include <blobpack/blobpack.h>
#include <libutype/avl.h>
#include <libutype/avl-cmp.h>

#include "juci_session.h"

struct juci {
	struct avl_tree objects; 
	struct avl_tree sessions; 
}; 

void juci_init(struct juci *self); 
int juci_load_plugins(struct juci *self, const char *path, const char *base_path); 
int juci_call(struct juci *self, const char *object, const char *method, struct blob_field *args, struct blob *out); 
int juci_list(struct juci *self, const char *path, struct blob *out); 
   
static inline bool url_scanf(const char *url, char *proto, char *host, int *port, char *page){
    if (sscanf(url, "%99[^:]://%99[^:]:%i/%199[^\n]", proto, host, port, page) == 4) return true; 
    else if (sscanf(url, "%99[^:]://%99[^/]/%199[^\n]", proto, host, page) == 3) return true; 
    else if (sscanf(url, "%99[^:]://%99[^:]:%i[^\n]", proto, host, port) == 3) return true; 
    else if (sscanf(url, "%99[^:]://%99[^\n]", proto, host) == 2) return true; 
    return false;                       
}   

