#include "test-funcs.h"
#include <stdbool.h>
#include <math.h>
#include <memory.h>
#include <blobpack/blobpack.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <utype/avl.h>
#include "../src/orange.h"
#include "../src/internal.h"

int main(void){
	orange_debug_level+=4; 

	struct orange *app = orange_new("test-plugins", "test-pwfile", "test-acls");
	struct blob out; 
	struct blob args; 
	blob_init(&out, 0, 0); 
	blob_init(&args, 0, 0); 

	blob_offset_t o = blob_open_table(&args); 
	blob_put_string(&args, "cmd"); 
	blob_put_string(&args, "echo 'Hello From Defferred!'"); 
	blob_put_string(&args, "msg"); 
	blob_put_string(&args, "Hello You"); 
	blob_put_string(&args, "arr"); 
	blob_offset_t a = blob_open_array(&args); 
	blob_put_int(&args, 1); 
	blob_offset_t t = blob_open_table(&args); 
	blob_put_string(&args, "a"); 
	blob_put_string(&args, "b"); 
	blob_close_table(&args, t); 
	blob_close_array(&args, a); 
	blob_close_table(&args, o); 

	struct orange_user *admin = orange_user_new("admin"); 
	orange_user_add_acl(admin, "test-acl"); 
	orange_add_user(app, &admin); 

	struct orange_sid sid; 
	TEST(orange_login_plaintext(app, "admin", "admin", &sid) == 0); 
	TEST(orange_call(app, sid.hash, "/test", "echo", blob_field_first_child(blob_head(&args)), &out) == 0);   
	TEST(orange_call(app, sid.hash, "/test", "noexist", blob_field_first_child(blob_head(&args)), &out) < 0);   
	TEST(orange_call(app, sid.hash, "/test", "test_c_calls", blob_field_first_child(blob_head(&args)), &out) == 0);   

	// test deferred
	struct blob def_args; 
	blob_init(&def_args, 0, 0); 
	o = blob_open_table(&def_args); 
	blob_put_string(&def_args, "cmd"); 
	char cmd[64]; 
	char cookie[16]; 
	snprintf(cookie, sizeof(cookie), "%lu", time(NULL)); 
	snprintf(cmd, sizeof(cmd), "printf %s > test-deferred.out", cookie); 
	blob_put_string(&def_args, cmd); 
	blob_close_table(&def_args, o); 
	int r = system("rm test-deferred.out"); 
	TEST(orange_call(app, sid.hash, "/test", "deferred_shell", blob_field_first_child(blob_head(&def_args)), &out) == 0);   
	FILE *f; 
	sleep(2); 
	TEST((f = fopen("test-deferred.out", "r")) == NULL); 
	sleep(3); 
	TEST(f = fopen("test-deferred.out", "r")); 
	// try to read the file
	char cmdr[32] = {0}; 
	TEST(fread(cmdr, 1, 32, f) > 0); 
	TEST(strcmp(cookie, cmdr) == 0); 
	blob_free(&def_args); 
	
	TEST(orange_logout(app, sid.hash) == 0); 

	blob_free(&out); 
	blob_free(&args); 
	orange_delete(&app); 

	return 0; 
}

