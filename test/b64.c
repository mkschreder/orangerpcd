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
	const char *str = "Hello Whatever"; // check step B 
	const char *str2 = "Hello The World"; // check step A
	const char *str3 = "Hello The World Online"; // check step C

	const char *bcheck = "SGVsbG8gV2hhdGV2ZXI="; 
	const char *bcheck2 = "SGVsbG8gVGhlIFdvcmxk"; 
	const char *bcheck3 = "SGVsbG8gVGhlIFdvcmxkIE9ubGluZQ=="; 

	int r = base64_encode(str, ebuf, sizeof(ebuf)); 

	printf("'%s' -> '%s' (%d)\n", str, ebuf, strlen(ebuf)); 
	
	TEST(strcmp(ebuf, bcheck) == 0); 
	TEST(r == strlen(ebuf)); 

	r = base64_decode(ebuf, dbuf, sizeof(dbuf)); 
	
	TEST(strcmp(dbuf, str) == 0); 
	TEST(r == strlen(dbuf)); 

	base64_encode(str2, ebuf, sizeof(ebuf)); 
	TEST(strcmp(ebuf, bcheck2) == 0); 
	base64_decode(ebuf, dbuf, sizeof(dbuf)); 
	TEST(strcmp(dbuf, str2) == 0); 

	base64_encode(str3, ebuf, sizeof(ebuf)); 
	TEST(strcmp(ebuf, bcheck3) == 0); 
	base64_decode(ebuf, dbuf, sizeof(dbuf)); 
	TEST(strcmp(dbuf, str3) == 0); 

	return 0; 
}
