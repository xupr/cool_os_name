#include "../headers/filesystem.h"

void init_tty(void){
	inode *ind;
	close(open("/dev/tty1", "r"));
	ind = get_inode("/dev/tty1");
	ind->type = SPECIAL_FILE;
	ind->device_type = TTY;
	close(open("/dev/tty2", "r"));
	ind = get_inode("/dev/tty2");
	ind->type = SPECIAL_FILE;
	ind->device_type = TTY;
	close(open("/dev/tty3", "r"));
	ind = get_inode("/dev/tty3");
	ind->type = SPECIAL_FILE;
	ind->device_type = TTY;
	close(open("/dev/tty4", "r"));
	ind = get_inode("/dev/tty4");
	ind->type = SPECIAL_FILE;
	ind->device_type = TTY;
	special_file_methods *sf_methods = (special_file_methods *)malloc(sizeof(special_file_methods));
	sf_methods->open = 0;
	sf_methods->read = 0;
	sf_methods->write = 0;
	sf_methods->close = 0;
	add_special_file_method(TTY, sf_methods);	
}
