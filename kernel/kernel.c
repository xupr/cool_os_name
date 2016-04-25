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
	cli();
	print_off();
	init_heap();
	//print("kernel heap initialized\n");
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
	/*print_to_other_screen("kappa123", 1);*/
//	FILE shell = open("shell.o");
//	write(shell, "123", 4);
//	char buff[32];
//	read(shell, buff, 32);
//	print(buff);
//PAGE_TABLE safta_table = create_page_table();
//	allocate_memory(safta_table, (void *)0xFFF, 32);
//	execute("shell.o", 1);
//	asm("sti");	
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
	//execute("shell.o", 0);
//	write(safta, "kappa123", 8);
	int i;
	for(i = 0; i < 1; ++i)
		execute("login.bin", i);

	/*PAGE_TABLE p = create_page_table();
	identity_page(p, 0, 0x5fffff);
	allocate_memory(p, 0x600000, 0x10000);
	PAGE_TABLE p2 = create_page_table();
	identity_page(p2, 0, 0x5fffff);
	allocate_memory(p2, 0x600000, 0x10000);
	free_page_table(p);
	free_page_table(p2);
	p = create_page_table();
	identity_page(p, 0, 0x5fffff);
	allocate_memory(p, 0x600000, 0x10000);
	p2 = create_page_table();
	identity_page(p2, 0, 0x5fffff);
	allocate_memory(p2, 0x600000, 0x10000);
	switch_memory_map(p);
	switch_memory_map(p2);*/
	/*PAGE_TABLE p = create_page_table();
	print(itoa(p));
	identity_page(p, 0, 0x5fffff);
	allocate_memory(p, 0x600000, 0x200000);
	free_page_table(p, 1);
	p = create_page_table();
	print(itoa(p));
	identity_page(p, 0, 0x5fffff);
	switch_memory_map(p);*/
//	print("231\n");
	sti();
	return;	
}

static int mutex = 0;
void cli(void){
	asm("cli");
	++mutex;
}

void sti(void){
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
