#include "../headers/process.h"
#include "../headers/memory.h"
#include "../headers/list.h"
#include "../headers/heap.h"
#include "../headers/screen.h"
#include "../headers/string.h"

#define PROCESS_CODE_BASE 0x600000
#define KERNEL_BASE 0x100000
#define KERNEL_LIMIT 0x4FFFFF

typedef struct{
	PAGE_TABLE pages;
	void *heap_start;
} process_descriptor;

static list *process_list;

void init_process(void){
	process_list = create_list();
	process_descriptor *kernel_process = (process_descriptor *)malloc(sizeof(process_descriptor));
	add_to_list(process_list, kernel_process);
}

void *get_heap_start(PID process_index){
	return ((process_descriptor *)get_list_element(process_list, process_index))->heap_start;
}

void create_process(char *code, int length){
	print(itoa(length));
	process_descriptor *process = (process_descriptor *)malloc(sizeof(process_descriptor));
	process->pages = create_page_table();
	process->heap_start = (void *)(PROCESS_CODE_BASE + length);
	add_to_list(process_list, process);
	identity_page(process->pages, (void *)KERNEL_BASE, KERNEL_LIMIT);
//	identity_page(process->pages, (void *)SCREEN, SCREEN_END - SCREEN - 1);
	identity_page(process->pages, (void *)0x0, 0xfffff);
	allocate_memory(process->pages, (void *)PROCESS_CODE_BASE, length);
	write_virtual_memory(process->pages, code, (void *)PROCESS_CODE_BASE, length);
//	print("allah");
	switch_memory_map(process->pages);
	//print(" akbar\n");
	asm("CALL EAX" : : "a"(PROCESS_CODE_BASE));
	switch_memory_map(KERNEL_PID);
}

PID get_current_process(void){
	return (PID)1;
}
