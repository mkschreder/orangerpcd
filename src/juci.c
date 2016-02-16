#include <stdio.h>
#include <unistd.h>
#include <dirent.h>

#include <blobpack/blobpack.h>
#include "juci.h"
#include "juci_luaobject.h"

int juci_debug_level = 0; 

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
		}
    }
    closedir(dir); 
    return rv; 
}

void juci_init(struct juci *self){
	avl_init(&self->objects, avl_strcmp, false, NULL); 
	avl_init(&self->sessions, avl_strcmp, false, NULL); 
}

int juci_call(struct juci *self, const char *object, const char *method, struct blob_field *args, struct blob *out){
	struct avl_node *avl = avl_find(&self->objects, object); 
	if(!avl) {
		ERROR("object not found: %s\n", object); 
		return -ENOENT; 
	}
	struct juci_luaobject *obj = container_of(avl, struct juci_luaobject, avl); 
	return juci_luaobject_call(obj, method, args, out); 
}

int juci_list(struct juci *self, const char *path, struct blob *out){
	struct juci_luaobject *entry; 
	blob_offset_t t = blob_open_table(out); 
	avl_for_each_element(&self->objects, entry, avl){
		blob_put_string(out, (char*)entry->avl.key); 
		blob_put_attr(out, blob_field_first_child(blob_head(&entry->signature))); 
	}
	blob_close_table(out, t); 
	return 0; 
}


