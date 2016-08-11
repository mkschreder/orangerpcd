#pragma once

#include <stdbool.h>

typedef struct JSON_check_struct {
	int state;
	int depth;
	int top;
	int* stack;
} * JSON_check;

JSON_check JSON_check_new(int depth);
bool JSON_check_string(JSON_check self, const char *str); 

