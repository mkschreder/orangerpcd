#include "test-funcs.h"
#include <stdbool.h>
#include <math.h>
#include <memory.h>
#include <blobpack/blobpack.h>
#include "../src/orange.h"

int main(){
	struct orange_user user; 

	struct orange_session *ses = orange_session_new(&user); 

	return 0; 
}
