#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define SECTOR_SIZE 512
#define BLOCK_SIZE 0x2000

#define FILESYSTEM_BASE 0x1100000
#define FREE_BLOCK_LIST_OFFEST 0x1100000
#define INODE_LIST_OFFSET 0x1102000
#define FILE_NAME_LIST_OFFSET 0x1104000
#define FILE_DATA_OFFSET 0x1108000

typedef enum {
	NOT_SPECIAL,
	TTY,
	NUM_DEVICE_TYPES
} DEVICE_TYPE;

typedef enum {
	NEW_FILE,
	REGULAR_FILE,
	DIRECTORY,
	SPECIAL_FILE
} FILE_TYPE;

typedef struct {
	unsigned char bound;
	FILE_TYPE type;
	DEVICE_TYPE major;
	unsigned int minor;
	unsigned short access;
	unsigned int creator_uid;
	unsigned int size;
	unsigned int address_block;
	unsigned short name_address;
	unsigned int creation_date;
	unsigned int update_date;
} inode;

int find_free_blocks(char *block_buff, int blocks_needed){
	int i, j, count = 0, start;
	for(i = 0; i < BLOCK_SIZE*8; ++i){
		if(((block_buff[i/8]>>(i%8))&1) == 0){
			if(count == 0)
				start = i;
			if(++count == blocks_needed)
				break;
		}else
			count = 0;
	}

	for(i = 0; i < blocks_needed; ++i)
		block_buff[(start + i)/8] = block_buff[(start + i)/8] | (1<<((start + i)%8));

	return FILESYSTEM_BASE + start*BLOCK_SIZE;
}

int main(int argc, char *argv[]){
	if(argc < 2){
		printf("%s\n", "usage: ./add_file.o [file name]");
	}

	char *file_name = argv[1];
	FILE *fd = fopen("./os.img", "r+");
	
	fseek(fd, INODE_LIST_OFFSET, SEEK_SET);
	inode *inode_list_buff = (inode *)malloc(BLOCK_SIZE);
	fread(inode_list_buff, 1, BLOCK_SIZE, fd);

	int i;
	for(i = 0; i < BLOCK_SIZE/sizeof(inode); ++i){
		if(!inode_list_buff[i].bound)
			break;
	}
	inode *current_inode = inode_list_buff+i;
	current_inode->bound = 1;
	current_inode->access = 0777;
	current_inode->type = DIRECTORY;
	current_inode->major = NOT_SPECIAL;
	current_inode->minor = 0;	
	current_inode->creator_uid = 0;
	current_inode->size = 0;

	fseek(fd, FILE_NAME_LIST_OFFSET, SEEK_SET);
	char *file_name_list_buff = (char *)malloc(BLOCK_SIZE);
	fread(file_name_list_buff, 1, BLOCK_SIZE, fd);

	for(i = 0; i < BLOCK_SIZE; ++i){
		if(file_name_list_buff[i] == 0 && file_name_list_buff[++i] == 0)
			break;
	}

	if(i == 1)
		--i;
	strcpy(file_name_list_buff + i, file_name);
	current_inode->name_address = i;
	fseek(fd, FILE_NAME_LIST_OFFSET, SEEK_SET);
	fwrite(file_name_list_buff, 1, BLOCK_SIZE, fd);

	fseek(fd, FREE_BLOCK_LIST_OFFEST, SEEK_SET);
	char *block_buff = (char *)malloc(BLOCK_SIZE);
	fread(block_buff, 1, BLOCK_SIZE, fd);
	
	int data_addresses_block_offset = find_free_blocks(block_buff, 1);
	current_inode->address_block = data_addresses_block_offset/SECTOR_SIZE;

	fseek(fd, FREE_BLOCK_LIST_OFFEST, SEEK_SET);
	fwrite(block_buff, 1, BLOCK_SIZE, fd);

	fseek(fd, INODE_LIST_OFFSET, SEEK_SET);
	fwrite(inode_list_buff, 1, BLOCK_SIZE, fd);
	fclose(fd);
}
