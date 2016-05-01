//#include "../headers/scancode.h"
#include "../headers/kernel.h"
#include "../headers/screen.h"
#include "../headers/interrupts.h"
#include "../headers/keyboard.h"
#include "../headers/heap.h"
#include "../headers/ata.h"
#include "../headers/string.h"
#include "../headers/filesystem.h"
#include "../headers/memory.h"
#include "../headers/process.h"
#include "../headers/system_call.h"
#include "../headers/exception.h"

void init(char *memory_map_length, void *memory_map);

void k_main(char *memory_map_length, void *memory_map){
	init(memory_map_length, memory_map);
	return;
}

void init(char *memory_map_length, void *memory_map){
	cli();
	init_heap(); //initialize everything
	init_screen();
	print("screen initialized\n");
	init_interrupts();
	print("interrupts initialized\n");
	init_exception();
	print("exceptions initialized\n");
	init_system_call();
	print("system calls initialized\n");
	init_memory(memory_map_length, memory_map);
	print("memory managment initialized\n");
	init_keyboard();
	print("keyboard initialized\n");
	init_ata();
	print("ata initialized\n");
	init_filesystem();
	print("file system initialized\n");
	init_process();
	print("process initialized\n");
	int i; //run all the login processes
	for(i = 0; i < 4; ++i)
		execute("login.bin", i, 0, 0);
	sti();
	return;	
}

static int mutex = 0;
void cli(void){ //clear interrupts
	asm("cli");
	++mutex;
}

void sti(void){ //set interrupts
	asm("cli");
	if(--mutex == 0){
		asm("sti");
		mutex = 0;
	}
}

void sti_forced(void){
	asm("cli");
	mutex = 0;
	asm("sti");
}
