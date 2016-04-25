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

FILE fopen(char *file_name, char *mode){
	FILE fd;
	asm("int 0x40" : "=a"(fd) : "a"(FOPEN), "b"(mode), "d"(file_name));	
	return fd;
}

int fwrite(char *buff, int length, FILE fd){
	int bytes;
	asm("int 0x40" : "=a"(bytes): "a"(FWRITE), "b"(fd), "d"(buff), "c"(length));
	return bytes;
}

int fread(char *buff, int length, FILE fd){
	int bytes;
	asm("int 0x40" : "=a"(bytes) : "a"(FREAD), "b"(fd), "d"(buff), "c"(length));
	return bytes;
}

void fclose(FILE fd){
	asm("pusha");
	asm("int 0x40" : : "a"(FCLOSE), "b"(fd));
	asm("popa");
}
