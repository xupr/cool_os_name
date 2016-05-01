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

void init_ata(void){ //initialize the ata
	create_IDT_descriptor(0x2E, (unsigned int)&ata_interrupt_entry, 0x8, 0x8F);
}

void ata_interrupt_handler(void){ //handle ata interrupts
	send_EOI(14);
	return;
}

void ata_read_sectors(int lba, char sector_count, char *buffer){ //read sectors
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

void ata_write_sectors(int lba, char sector_count, char *buffer){ //write sectors
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
