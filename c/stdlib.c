#include "headers/stdlib.h"
#include "../headers/system_call.h"
#include "headers/stdio.h"
#include "headers/string.h"

void *heap;

typedef struct _heap_memory_structure {
	unsigned char free;
	unsigned int size;
	struct _heap_memory_structure *next;
} heap_memory_structure;

void init_heap(void){ //initilize the heap
	asm("int 0x40" : "=a"(heap): "a"(HEAP_START));
	heap_memory_structure *s = (heap_memory_structure *)heap;
	s->free = 1;
	s->size = -1;
	s->next = 0;
	
	return;
}

void exit(int result_code){ //exit the process
	asm("int 0x40" : : "a"(EXIT), "d"(result_code));
}

void free(void *  heap_address){ //free memory from the heap
	heap_memory_structure *s = heap_address - sizeof(heap_memory_structure);
	s->free = 1;
	return;
}

void *malloc(int size){ //alocate memory from the heap
	heap_memory_structure *s = (heap_memory_structure *)heap;
	while((s != 0) && (s->free != 1 || s->size < size)) s = s->next;
	if(s != 0){
		if(s->size = -1)
			s->size = size;
		if(s->next == 0){
			s->next = (heap_memory_structure *)((char *) s + sizeof(heap_memory_structure) + s->size);
			s->next->free = 1;
			s->next->size = -1;
			s->next->next = 0;
		}
		
		s->free = 0;
	}

	return (void *)s + sizeof(heap_memory_structure);
}
