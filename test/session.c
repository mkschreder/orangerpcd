#include "test-funcs.h"
#include <stdbool.h>
#include <math.h>
#include <memory.h>
#include <blobpack/blobpack.h>
#include "../src/orange.h"

int main(){
	struct orange_user user; 

	struct orange_session *ses = orange_session_new(&user); 
	
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

	TEST(!orange_session_access(ses, "uci", "network", "*", "rwx")); 
	TEST(!orange_session_access(ses, "uci", "network", "*", "x")); 
	TEST(orange_session_grant(ses, "uci", "net*", "*", "x") == 0); 
	TEST(orange_session_access(ses, "uci", "network", "*", "x")); 

	return 0; 
}
