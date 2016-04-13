#include "headers/stdio.h"
#include "../headers/system_call.h"
#include "headers/string.h"

void print(char *str){
	asm("pusha");
	asm("int 0x40" : : "d"(str), "a"(PRINT));
	asm("popa");
}

void input(char *str, int length){
	asm("pusha");
	asm("int 0x40" : : "d"(str), "a"(INPUT), "c"(length));
	asm("popa");
}

FILE fopen(char *file_name){
	FILE fd;
	asm("int 0x40" : "=a"(fd) : "a"(FOPEN), "d"(file_name));	
	return fd;
}

void fwrite(char *buff, int length, FILE fd){
	asm("pusha");
	asm("int 0x40" : : "a"(FWRITE), "b"(fd), "d"(buff), "c"(length));
	asm("popa");
}

void fread(char *buff, int length, FILE fd){
	asm("pusha");
	asm("int 0x40" : : "a"(FREAD), "b"(fd), "d"(buff), "c"(length));
	asm("popa");
}
