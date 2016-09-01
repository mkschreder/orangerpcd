#include "test-funcs.h"
#include <stdbool.h>
#include <math.h>
#include <memory.h>
#include <blobpack/blobpack.h>
#include <libutype/avl.h>
#include "../src/orange.h"
#include "../src/orange_id.h"
#include "../src/base64.h"

int main(){
	char ebuf[1000]; 
	char dbuf[1000]; 
	const char *str = "Hello Whatever"; 
	const char *bcheck = "SGVsbG8gV2hhdGV2ZXI="; 

	int r = base64_encode(str, ebuf, sizeof(ebuf)); 

	printf("'%s' -> '%s' (%d)\n", str, ebuf, strlen(ebuf)); 
	
	TEST(strcmp(ebuf, bcheck) == 0); 
	TEST(r == strlen(ebuf)); 

	r = base64_decode(ebuf, dbuf, sizeof(dbuf)); 
	
	TEST(strcmp(dbuf, str) == 0); 
	TEST(r == strlen(dbuf)); 

	return 0; 
}
