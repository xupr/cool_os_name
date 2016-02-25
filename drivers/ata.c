#include "../headers/ata.h"
#include "../headers/portio.h"
#include "../headers/screen.h"
#include "../headers/heap.h"
#include "../headers/interrupts.h"
#include "../headers/string.h"

#define ATA_DATA_REGISTER_PORT 0x1F0
#define ATA_SECTOR_COUNT_REGISTER_PORT 0x1F2
#define ATA_LBA_LOW_REGISTER_PORT 0x1F3
#define ATA_LBA_MID_REGISTER_PORT 0x1F4
#define ATA_LBA_HIGH_REGISTER_PORT 0x1F5
#define ATA_DRIVE_REGISTER_PORT 0x1F6
#define ATA_COMMAND_REGISTER_PORT 0x1F7
#define ATA_REGULAR_STATUS_REGISTER_PORT 0x1F7

#define ATA_IDENTIFY_DEVICE 0xEC
#define ATA_READ 0x20
#define ATA_WRITE 0x30

void *ata_interrupt_entry;
//void ata_read_blocks(int, int, char *);
//void ata_write_blocks(int, int, char *);

unsigned int ata_get_sector_count(void){
	unsigned int sector_count = 0;
	outb(ATA_DRIVE_REGISTER_PORT, 0xA0); //select the master drive
	outb(ATA_LBA_LOW_REGISTER_PORT, 0); //set other registers to 0 to check for non ata devices
	outb(ATA_LBA_MID_REGISTER_PORT, 0);
	outb(ATA_LBA_HIGH_REGISTER_PORT, 0);
	outb(ATA_SECTOR_COUNT_REGISTER_PORT, 0);
	outb(ATA_COMMAND_REGISTER_PORT, ATA_IDENTIFY_DEVICE); //send the identify command
	while(inb(ATA_REGULAR_STATUS_REGISTER_PORT) & 0b10000000); //wait untill not BSY
	if(inb(ATA_REGULAR_STATUS_REGISTER_PORT) & 0b1) print("error occured");
	else if(inb(ATA_LBA_HIGH_REGISTER_PORT) != 0 || inb(ATA_LBA_MID_REGISTER_PORT) != 0) print("not an ata device");
	else if(inb(ATA_REGULAR_STATUS_REGISTER_PORT) & 0b1000){
		unsigned short *identify_buffer = (unsigned short *)malloc(256*sizeof(short));
		int i = 0;
		for(; i < 256; *(identify_buffer + (i++)) = inw(ATA_DATA_REGISTER_PORT));
		sector_count = (int)(*(identify_buffer + 60));
		free(identify_buffer);
	}
	
	return sector_count;
}

void init_ata(void){
	//print(itoa(123));
	create_IDT_descriptor(0x2E, (unsigned int)&ata_interrupt_entry, 0x8, 0x8F);
//	outb(0x3F6, 0);
/*	unsigned char status_register = inb(0x1F7);
	print(itoa(status_register));
	print("\n");
	outb(0x3F6, 0);
	unsigned char s = inb(0x1F7);
	status_register = inb(0x1F7);
	print(itoa(status_register));
	print("\n");
	while(inb(0x3F6) & 0b10000000);
	outb(0x3F6, 0);
	status_register = inb(0x1F7);
	print(itoa(status_register));
	print("\n");
	if(s == 0xFF) print("well, fuck");
	outb(0x1F6, 0xA0);
	outb(0x1F2, 0);
	outb(0x1F3, 0);
	outb(0x1F4, 0);
	outb(0x1F5, 0);
	outb(0x1F7, 0xEC);
	while(inb(0x1F7) & 0b10000000);
	status_register = inb(0x1F7);
	print(itoa(status_register));
	print("\n");
	if(status_register == 0) print("I fucked up");
	if(status_register & 1) print("someone just dungoofed");
	if(status_register & 0b10000000) print("wtf bochs?");
	else print("safta is life");
	if(status_register & 0b100000) print("rekt son");
	if(inb(0x1F5) != 0) print("wasnt this suppoused to be ata?");
	if(inb(0x1F4) != 0) print("^");
	if(!(status_register & 0b1000)) print("safta shelahem alai");
	while(!(inb(0x1F7) & 0b1000));
	int i = 0;
	unsigned short *identety = (unsigned short *)malloc(256*sizeof(unsigned short));
	for(; i < 256; identety[i++]=inw(0x1F0));
	int safta = *((unsigned int *)(identety+60)); 
//	print(itoa((int)(*(identety+255))));
	print("\n");
	print(itoa(safta));
	print("done");*/

/*	char *buffer = (char *)malloc(512*sizeof(char));
	*buffer = 'h';
	*(buffer+1) = 'e';
	*(buffer+2) = 'l';
	*(buffer+3) = 'l';
	*(buffer+4) = 'o';
	*(buffer+5) = '!';
	*(buffer+6) = '\0';
	ata_write_blocks(0, 1, buffer);
	*buffer = 's';
	ata_read_blocks(0, 1, buffer);
	print(buffer);*/
}

void ata_interrupt_handler(void){
	print("hello is it me it was looking for?");
	send_EOI(14);
	return;
}

void ata_read_sectors(int lba, char sector_count, char *buffer){
	outb(ATA_DRIVE_REGISTER_PORT, 0xE0 | 0x40 | ((lba>>24) & 0x0F)); //master drive + lba + high lba bits
	outb(ATA_SECTOR_COUNT_REGISTER_PORT, (unsigned char)sector_count); //sector count
	outb(ATA_LBA_LOW_REGISTER_PORT, (unsigned char)lba);
	outb(ATA_LBA_MID_REGISTER_PORT, (unsigned char)(lba>>8));
	outb(ATA_LBA_HIGH_REGISTER_PORT, (unsigned char)(lba>>16));
	outb(ATA_COMMAND_REGISTER_PORT, ATA_READ); //read command
	int offset = 0;
	for(;sector_count--;){
		while(inb(ATA_REGULAR_STATUS_REGISTER_PORT) & 0b10000000); //wait until not BSY
		while((~inb(ATA_REGULAR_STATUS_REGISTER_PORT)) & 0b1000); //wait until DRQ set
		int i = 0;
		for(;i < 256;*((unsigned short *)buffer + offset + i++)=inw(ATA_DATA_REGISTER_PORT));
		offset += 256;
	}
	return;
}

void ata_write_sectors(int lba, char sector_count, char *buffer){
	outb(ATA_DRIVE_REGISTER_PORT, 0xE0 | 0x40 | ((lba>>24) & 0x0F)); //master drive + lbal + high lba bits
	outb(ATA_SECTOR_COUNT_REGISTER_PORT, (unsigned char)sector_count);
	outb(ATA_LBA_LOW_REGISTER_PORT, (unsigned char)lba);
	outb(ATA_LBA_MID_REGISTER_PORT, (unsigned char)(lba>>8));
	outb(ATA_LBA_HIGH_REGISTER_PORT, (unsigned char)(lba>>16));
	outb(ATA_COMMAND_REGISTER_PORT, ATA_WRITE);
	int offset = 0;
	for(;sector_count--;){
		while(inb(ATA_REGULAR_STATUS_REGISTER_PORT) & 0b10000000);
		while((~inb(ATA_REGULAR_STATUS_REGISTER_PORT)) & 0b1000);
		int i = 0;
		for(;i < 256; outw(ATA_DATA_REGISTER_PORT, *((unsigned short *)buffer + offset + i++)));
		offset += 256;
}
	return;
}
