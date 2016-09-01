#include "test-funcs.h"
#include <stdbool.h>
#include <math.h>
#include <memory.h>
#include <blobpack/blobpack.h>
#include "../src/orange.h"
#include "../src/orange_ws_server.h"

int main(){
	orange_server_t s = orange_ws_server_new("test-www"); 

	// TODO: use node client to test this script

	return 0; 
}
