#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define INODE_OFFSET 0x1102000
#define FILE_DATA_OFFSET 0x1108000

typedef struct {
	unsigned char bound;
	unsigned short access;
	unsigned int size;
	unsigned int address_block;
	unsigned short name_address;
	unsigned int creation_date;
	unsigned int update_date;
} inode;

int main(void){
	struct stat *buff = (struct stat *)malloc(sizeof(struct stat));
	stat("./shell.bin", buff);
	printf("%d\n", buff->st_size);
	FILE *fd = fopen("./os_copy.img", "r+");
	fseek(fd, INODE_OFFSET, SEEK_SET);
	inode *ind = (inode *)malloc(sizeof(inode));
	fgets((char *)ind, sizeof(inode), fd);
	printf("%d\n", ind->name_address);
	ind->size = buff->st_size + 1;
	fseek(fd, INODE_OFFSET, SEEK_SET);
	fwrite(ind, sizeof(inode), 1, fd);
/*	char i[24];
	itoa(FILE_DATA_OFFSET + buff->st_size, i, 16);
	printf("%s\n", i);
*/	fseek(fd, FILE_DATA_OFFSET + buff->st_size, SEEK_SET);
//	printf("%d\n", fgetc(fd));	
	char i = 255;
	fwrite(&i, 1, 1, fd);
}
