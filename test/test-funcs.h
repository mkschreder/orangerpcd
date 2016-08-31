#pragma once

#include <math.h>

#ifndef FLT_EPSILON
#define FLT_EPSILON 1e-5
#endif

static inline unsigned char is_equal(float a, float b){
	return fabsf(a - b) < FLT_EPSILON; 
}

#define STR(x) #x                       
#define TEST(x) if(!(x)){ printf("test failed at %d, %s: %s\n", __LINE__, __FILE__, STR(x)); exit(-1); } else { printf("[OK] %s\n", STR(x)); }
