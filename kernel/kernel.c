//#include "../headers/scancode.h"
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
//	print(itoa((int)memory_map));
//	print("safta\n");
	init(memory_map_length, memory_map);
//	print(itoa(*memory_map_length));
//	print("\n");
	//print("Hello World!\n");
	//print("safta\n");
	return;
}

void init(char *memory_map_length, void *memory_map){
	init_screen();
	print("screen initialized\n");
	init_heap();
	print("kernel heap initialized\n");
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
	asm("sti");	
	//PAGE_TABLE safta_table = create_page_table();
//	allocate_memory(safta_table, (void *)0xFFF, 32);
	execute("safta.o");
//	open("safta.txt");
//	open("saba.txt");
/*	FILE safta = open("safta.txt");	
	//print(itoa((unsigned int)safta));
	write(safta, "hello this is safta", 20);
	write(safta, "this is getting crazy", 22);
	seek(safta, 0);
	char buff[22];
	read(safta, buff, 20);
	print(buff);
	read(safta, buff, 22);
	print(buff);*/
//	FILE safta = open("safta.o");
//	execute("safta.o");
//	write(safta, "kappa123", 8);

//	print("231\n");
	return;	
}
