#pragma once

int base64_decode(const char* code_in, const int length_in, char* plaintext_out); 
int base64_encode(const char* text_in, char* code_out, const int size_out); 
