#include "../headers/heap.h"
#include "../headers/screen.h"

#define HEAP 0x200000

typedef struct _heap_memory_structure {
	unsigned char free;
	unsigned int size;
	struct _heap_memory_structure *next;
} heap_memory_structure;

void init_heap(void){ //initialize the heap
	heap_memory_structure *s = (heap_memory_structure *)HEAP;
	s->free = 1;
	s->size = -1;
	s->next = 0;
	return;
}

void *malloc(unsigned int size){ //allocate memory from the heap
	heap_memory_structure *s = (heap_memory_structure *)HEAP;
	while((s != 0) && (s->free != 1 || s->size < size))
		s = s->next;
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

void free(void* heap_address){ //free memory from the heap
	heap_memory_structure *s = heap_address - sizeof(heap_memory_structure);
	s->free = 1;
	return;
}
