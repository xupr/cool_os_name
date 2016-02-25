#include "../headers/portio.h"

unsigned char inb(unsigned short port){
	unsigned char result;
	asm("in al, dx" : "=a" (result) : "d" (port));
	return result;
}

void outb(unsigned short port, unsigned char value){
	asm("out dx, al" : : "d" (port), "a" (value));	
}

unsigned short inw(unsigned short port){
	unsigned short result;
	asm("in ax, dx" : "=a" (result) : "d" (port));
	return result;
}

void outw(unsigned short port, unsigned short value){
	asm("out dx, ax" : : "d" (port), "a" (value));	
}
