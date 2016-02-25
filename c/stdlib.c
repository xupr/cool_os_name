#include "headers/stdlib.h"
#include "../headers/system_call.h"
#include "headers/stdio.h"

void *malloc(int size){
	static void *heap_start = 0;
	print("hello\n");
	if(!heap_start){
		asm("int 0x40" : : "a"(HEAP_START), "d"(&heap_start));
	}
}
