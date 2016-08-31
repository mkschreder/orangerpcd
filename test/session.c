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
	
	return 0; 
}
