#include "../headers/filesystem.h"
#include "../headers/string.h"
#include "../headers/screen.h"
#include "../headers/keyboard.h"

void tty_open(char *file_name, char *mode, file_descriptor *fd){
	*((int *)fd->physical_address_block) = (int)(file_name[strlen(file_name) - 1]) - 0x31;	
}

int tty_read(char *buff, int count, file_descriptor *fd){
	input(buff, count, *((int *)fd->physical_address_block));		
	return strlen(buff);
}

int tty_write(char *buff, int count, file_descriptor *fd){
	set_vga_colors(WHITE, BLACK);
	print_to_other_screen(buff, *((int *)fd->physical_address_block));
	return strlen(buff);
}

void init_tty(void){
	cli();
	inode *ind;
	if(!(ind = get_inode("/dev/tty1"))){
		close(open("/dev/tty1", "r"));
		ind = get_inode("/dev/tty1");
	}
	ind->type = SPECIAL_FILE;
	ind->device_type = TTY;
	ind->access = 0666;
	if(!(ind = get_inode("/dev/tty2"))){
		close(open("/dev/tty2", "r"));
		ind = get_inode("/dev/tty2");
	}
	ind->type = SPECIAL_FILE;
	ind->device_type = TTY;
	ind->access = 0666;
	if(!(ind = get_inode("/dev/tty3"))){
		close(open("/dev/tty3", "r"));
		ind = get_inode("/dev/tty3");
	}
	ind->type = SPECIAL_FILE;
	ind->device_type = TTY;
	ind->access = 0666;
	if(!(ind = get_inode("/dev/tty4"))){
		close(open("/dev/tty4", "r"));
		ind = get_inode("/dev/tty4");
	}
	ind->type = SPECIAL_FILE;
	ind->device_type = TTY;
	ind->access = 0666;
	special_file_methods *sf_methods = (special_file_methods *)malloc(sizeof(special_file_methods));
	sf_methods->open = tty_open;
	sf_methods->read = tty_read;
	sf_methods->write = tty_write;
	sf_methods->close = 0;
	sf_methods->seek = 0;
	add_special_file_method(TTY, sf_methods);	
	sti();
}
