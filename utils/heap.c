#include "../headers/heap.h"
#include "../headers/screen.h"

#define HEAP 0x200000

typedef struct _heap_memory_structure {
	unsigned char free;
	unsigned int size;
	struct _heap_memory_structure *next;
} heap_memory_structure;

void init_heap(void){
	heap_memory_structure *s = (heap_memory_structure *)HEAP;
	s->free = 1;
	s->size = -1;
	s->next = 0;
	return;
}

void *malloc(unsigned int size){
	heap_memory_structure *s = (heap_memory_structure *)HEAP;
	while((s != 0) && (s->free != 1 || s->size < size)){ 
		/*if(s->next > 0x300000){
			print_on();
			print("asdasdasd!");
			print_off();
		}*/
		s = s->next;
	}
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
	/*if((void *)s + sizeof(heap_memory_structure) + size>0x300000){
		print_on();
		print("WTFFFFFFFFFFFFFFFF");
		print_off();
	}*/
	/*print(itoa(s));*/
	return (void *)s + sizeof(heap_memory_structure);
}

void free(void* heap_address){
	heap_memory_structure *s = heap_address - sizeof(heap_memory_structure);
	s->free = 1;
	return;
}
