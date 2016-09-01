#include "test-funcs.h"
#include <stdbool.h>
#include <math.h>
#include <memory.h>
#include <blobpack/blobpack.h>
#include <libutype/avl.h>
#include "../src/orange.h"
#include "../src/internal.h"

int main(){
	orange_debug_level+=4; 

	struct orange *app = orange_new("test-plugins", "test-pwfile", "test-acls");
	struct blob out; 
	struct blob args; 
	blob_init(&out, 0, 0); 
	blob_init(&args, 0, 0); 

	blob_offset_t o = blob_open_table(&args); 
	blob_put_string(&args, "msg"); 
	blob_put_string(&args, "Hello You"); 
	blob_close_table(&args, o); 

	struct orange_user *admin = orange_user_new("admin"); 
	orange_user_add_acl(admin, "test-acl"); 
	orange_add_user(app, &admin); 

	struct orange_sid sid; 
	TEST(orange_login_plaintext(app, "admin", "admin", &sid) == 0); 
	TEST(orange_call(app, sid.hash, "/test", "echo", blob_field_first_child(blob_head(&args)), &out) == 0);   
	TEST(orange_logout(app, sid.hash) == 0); 

	orange_delete(&app); 

	return 0; 
}

