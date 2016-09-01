#include "test-funcs.h"
#include <stdbool.h>
#include <math.h>
#include <memory.h>
#include <blobpack/blobpack.h>
#include <libutype/avl.h>
#include "../src/orange.h"
#include "../src/orange_id.h"

int main(){
	struct avl_tree t; 
	struct orange_id n1, n2, n3, n4; 

	orange_id_tree_init(&t); 
	orange_id_alloc(&t, &n1, 0); 
	printf("allocated id %d\n", n1.id); 
	orange_id_alloc(&t, &n2, 0); 
	printf("allocated id %d\n", n2.id); 

	orange_id_alloc(&t, &n3, 123); 

	TEST(orange_id_find(&t, n1.id) == &n1); 
	TEST(orange_id_find(&t, n2.id) == &n2); 
	TEST(orange_id_find(&t, 123) == &n3); 
	TEST(n1.id != n2.id && n2.id != n3.id && n1.id != n3.id); 

	orange_id_free(&t, &n1); 
	orange_id_free(&t, &n2); 

	return 0; 
}

