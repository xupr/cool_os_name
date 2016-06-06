#include "../headers/stdlib.h"
#include "../headers/string.h"
#include "../headers/heap.h"
#include "../headers/screen.h"

unsigned int abs(int a){
	if(a < 0)
		return -a;
	return a;
}

void sort(void *start, int size, int length, int (*compare)(void *a, void *b)){
	char sorted = 0;
	void *temp = malloc(size);
	while(!sorted){
		sorted = 1;
		int i;
		for(i = 1; i < length; ++i){
			void *a = start + (i - 1)*size, *b = start + i*size;
			int result = compare(a, b);
			if(result > 0){
				memcpy(temp, a, size);
				memcpy(a, b, size);
				memcpy(b, temp, size);
				sorted = 0;
			}
		}
	}
}
