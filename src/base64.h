#pragma once

#include <stddef.h>

int base64_decode(const char* code_in, char* plaintext_out, size_t size_out); 
int base64_encode(const char* text_in, char* code_out, size_t size_out); 
