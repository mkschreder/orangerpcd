#pragma once

#include <stddef.h>

int base64_decode(const char* code_in, char* plaintext_out, size_t size_out); 
int base64_encode(const char* text_in, char* code_out, size_t size_out); 

#define B64_ENCODE_LEN(_len)	((((_len) + 2) / 3) * 4 + 1)
#define B64_DECODE_LEN(_len)	(((_len) / 4) * 3 + 1)


