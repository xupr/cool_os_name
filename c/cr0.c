#include "headers/stdlib.h"

extern int main();

void init(void){
	init_heap();
	main();
}
