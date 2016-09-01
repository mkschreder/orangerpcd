#include "test-funcs.h"
#include <stdbool.h>
#include <math.h>
#include <memory.h>
#include <blobpack/blobpack.h>
#include "../src/orange.h"
#include "../src/sha1.h"

int main(){
	unsigned char binhash[SHA1_BLOCK_SIZE+1] = {0}; 
	const char *input1 = "Hello"; 
	const char *input2 = "-dlroW"; 
	const char *checkhash = "87fd1a1a72301c45e4d852304783d0871f0c75bf"; 

	SHA1_CTX ctx; 
	sha1_init(&ctx); 
	sha1_update(&ctx, input1, strlen(input1)); 
	sha1_update(&ctx, input2, strlen(input2)); 
	sha1_final(&ctx, binhash); 
	char hash[SHA1_BLOCK_SIZE*2+1] = {0}; 
	for(int c = 0; c < SHA1_BLOCK_SIZE; c++) sprintf(hash + (c * 2), "%02x", binhash[c]); 
	
	TEST(strcmp(hash, checkhash) == 0); 

	return 0; 
}
