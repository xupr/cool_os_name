#include "../headers/filesystem.h"
#include "../headers/string.h"
#include "../headers/screen.h"
#include "../headers/keyboard.h"

int tty_read(char *buff, int count, file_descriptor *fd){ //reads from terminal
	input(buff, count, fd->inode->minor);		
	return strlen(buff);
}

int tty_write(char *buff, int count, file_descriptor *fd){ //writes to terminal
	set_vga_colors(WHITE, BLACK);
	print_to_other_screen(buff, fd->inode->minor);
	return strlen(buff);
}

void init_tty(void){ //initializes the terminal special file
	cli();
	inode *ind;
	if(!(ind = get_inode("/dev/tty1"))){
		close(open("/dev/tty1", "r"));
		ind = get_inode("/dev/tty1");
	}
	ind->type = SPECIAL_FILE;
	ind->major = TTY;
	ind->minor = 0;
	ind->access = 0666;
	if(!(ind = get_inode("/dev/tty2"))){
		close(open("/dev/tty2", "r"));
		ind = get_inode("/dev/tty2");
	}
	ind->type = SPECIAL_FILE;
	ind->major = TTY;
	ind->minor = 1;
	ind->access = 0666;
	if(!(ind = get_inode("/dev/tty3"))){
		close(open("/dev/tty3", "r"));
		ind = get_inode("/dev/tty3");
	}
	ind->type = SPECIAL_FILE;
	ind->major = TTY;
	ind->minor = 2;
	ind->access = 0666;
	if(!(ind = get_inode("/dev/tty4"))){
		close(open("/dev/tty4", "r"));
		ind = get_inode("/dev/tty4");
	}
	ind->type = SPECIAL_FILE;
	ind->major = TTY;
	ind->minor = 3;
	ind->access = 0666;
	special_file_methods *sf_methods = (special_file_methods *)malloc(sizeof(special_file_methods));
	sf_methods->open = 0;
	sf_methods->read = tty_read;
	sf_methods->write = tty_write;
	sf_methods->close = 0;
	sf_methods->seek = 0;
	add_special_file_method(TTY, sf_methods);	
	sti();
}
