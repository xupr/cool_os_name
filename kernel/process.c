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
	PAGE_TABLE page_table;
	void *heap_start;
} process_descriptor;

typedef struct{
	char present : 1;
	char wrtie : 1;
	char user : 1;
	char reserved_write : 1;
	char instruction_fetch : 1;
} page_fault_error_code;

static list *process_list;
static char tss[104];
static int current_process = 1;
//void (*jump_to_ring3)(int offset);
extern void jump_to_ring3(int offset);

void init_process(void){
	char gdt[6];
	asm("sgdt %0" : "=m"(gdt));
	print("\n");
	char *c = *((char **)(gdt + 2)) + 5*8;
	print(itoa((int)c));
	print("asd\n");
	*(short *)(c+2) = (short)((int)tss & 0xFFFF);
	*(c+4) = (char)(((int)tss>>16) & 0xFF);
	*(c+7) = (char)(((int)tss>>24) & 0xFF);
	print(itoa((int)tss));
	*(int *)(tss + 4) = 0x4fffff;
	*(short *)(tss + 8) = 0x10;
	*(short *)(tss + 102) = 104;
	asm("mov ax, 0x2B; ltr ax");

	process_list = create_list();
	process_descriptor *kernel_process = (process_descriptor *)malloc(sizeof(process_descriptor));
	add_to_list(process_list, kernel_process);
}

void handle_page_fault(void *address, int fault_info){
	if(!(*(page_fault_error_code *)&fault_info).present){
		print("page not present");
		process_descriptor *current_process_descriptor = (process_descriptor *)get_list_element(process_list, current_process);
		allocate_memory(current_process_descriptor->page_table, address, 1);
	}else{
		print("no premissions to enter page");
stop:
		asm("hlt");
		goto stop;
	}
}

void *get_heap_start(PID process_index){
	return ((process_descriptor *)get_list_element(process_list, process_index))->heap_start;
}

void create_process(char *code, int length){
	print(itoa(length));
	process_descriptor *process = (process_descriptor *)malloc(sizeof(process_descriptor));
	process->page_table = create_page_table();
	process->heap_start = (void *)(PROCESS_CODE_BASE + length);
	add_to_list(process_list, process);
	identity_page(process->page_table, (void *)KERNEL_BASE, KERNEL_LIMIT);
//	identity_page(process->page_table, (void *)SCREEN, SCREEN_END - SCREEN - 1);
	identity_page(process->page_table, (void *)0x0, 0xfffff);
	allocate_memory(process->page_table, (void *)PROCESS_CODE_BASE, length);
	write_virtual_memory(process->page_table, code, (void *)PROCESS_CODE_BASE, length);
//	print("allah");
	switch_memory_map(process->page_table);
	//print(" akbar\n");
	jump_to_ring3(PROCESS_CODE_BASE);
	//asm("JMP 0x18:0x600000"); //: : "a"(PROCESS_CODE_BASE));
	//switch_memory_map(KERNEL_PID);
}

PID get_current_process(void){
	return (PID)1;
}
