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

void init(char *memory_map_length, void *memory_map);

void k_main(char *memory_map_length, void *memory_map){
	print(itoa((int)memory_map));
	print("safta\n");
	init(memory_map_length, memory_map);
//	print(itoa(*memory_map_length));
//	print("\n");
	//print("Hello World!\n");
	//print("safta\n");
	return;
}

void init(char *memory_map_length, void *memory_map){
	init_heap();
	init_interrupts();
	init_screen();
	init_system_call();
	init_memory(memory_map_length, memory_map);
	init_keyboard();
	init_ata();
	init_filesystem();
	init_process();
	asm("sti");	
	//PAGE_TABLE safta_table = create_page_table();
//	allocate_memory(safta_table, (void *)0xFFF, 32);
	execute("safta.o");
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
