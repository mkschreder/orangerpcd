#include "test-funcs.h"
#include <stdbool.h>
#include <math.h>
#include <memory.h>
#include <blobpack/blobpack.h>
#include "../src/orange.h"

int main(){
	struct orange_user user; 

	struct orange_session *ses = orange_session_new(&user, 20); 

	TEST(orange_session_grant(ses, "myscope", "myobject", "mymethod", "r") == 0); 
	TEST(orange_session_access(ses, "myscope", "myobject", "mymethod", "r")); 
	TEST(!orange_session_access(ses, "myscope", "myobject", "mymethod", "w")); 
	TEST(orange_session_grant(ses, "myscope", "myobject", "mymethod", "w") == 0); 
	TEST(orange_session_access(ses, "myscope", "myobject", "mymethod", "r")); 
	TEST(orange_session_access(ses, "myscope", "myobject", "mymethod", "w")); 
	
	TEST(orange_session_grant(ses, "uci", "net*", "*", "rw") == 0); 
	TEST(orange_session_access(ses, "uci", "network", "*", "w")); 
	TEST(!orange_session_access(ses, "uci", "newwork", "*", "w")); 
	TEST(orange_session_access(ses, "uci", "network", "*", "rw")); 

	// try converting to blob and print as json
	struct blob b; 
	blob_init(&b, 0, 0); 
	orange_session_to_blob(ses, &b); 
	blob_dump_json(&b); 
	blob_free(&b); 

	// test that we do not have rwx access
	TEST(!orange_session_access(ses, "uci", "network", "*", "rwx")); 
	TEST(!orange_session_access(ses, "uci", "network", "*", "x")); 
	// grant x using wildcard
	TEST(orange_session_grant(ses, "uci", "net*", "*", "x") == 0); 
	// test wildcard
	TEST(orange_session_access(ses, "uci", "network", "*", "x")); 
	TEST(orange_session_access(ses, "uci", "netbark", "*", "x")); 

	// test revoke
	TEST(orange_session_revoke(ses, "uci", "netbark", "*", "x") == 0); 
	TEST(!orange_session_access(ses, "uci", "netbark", "*", "x")); 
	// try removing using wildcard
	TEST(orange_session_revoke(ses, "uci", "net*", "*", "w") == 0); 
	TEST(!orange_session_access(ses, "uci", "network", "*", "w")); 
	// read access should be still there
	TEST(orange_session_access(ses, "uci", "network", "*", "r")); 

	orange_session_delete(&ses); 

	return 0; 
}
