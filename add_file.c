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
	char *temp_buff = (char *)malloc(strlen(file_name));
	strcpy(temp_buff, file_name);
	char *inserted_file_name = strtok(temp_buff, "/\n"), *_prev = inserted_file_name;
	while(inserted_file_name = strtok(0, "/\n"))
		_prev = inserted_file_name;	
	inserted_file_name = _prev;

	char *_inserted_file_name = (char *)malloc(strlen(inserted_file_name) + 1);
	strcpy(_inserted_file_name, inserted_file_name);
	char *extension = strtok(_inserted_file_name, ".");
	extension = strtok(NULL, ".");
	if(!strcmp(extension, "bin"))
		strtok(inserted_file_name, ".");

	struct stat *stat_buff = (struct stat *)malloc(sizeof(struct stat));
	stat(file_name, stat_buff);
	int file_size = stat_buff->st_size;
	printf("%s, %d\n", file_name, file_size);
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
	
	fseek(fd, inode_list_buff[0].address_block*SECTOR_SIZE, SEEK_SET);
	int *inode_index_list_buff = (int *)malloc(BLOCK_SIZE);
	fread(inode_index_list_buff, 1, BLOCK_SIZE, fd);
	int j;
	for(j = 0; j < BLOCK_SIZE/sizeof(int); ++j){
		if(inode_index_list_buff[j] == 0){
			inode_index_list_buff[j] = i;
			break;
		}
	}
	fseek(fd, inode_list_buff[0].address_block*SECTOR_SIZE, SEEK_SET);
	fwrite(inode_index_list_buff, 1, BLOCK_SIZE, fd);

	current_inode->bound = 1;
	if(!strcmp(extension, "bin"))
		current_inode->access = 0777;
	else
		current_inode->access = 0700;
	current_inode->type = REGULAR_FILE;
	current_inode->major = NOT_SPECIAL;
	current_inode->minor = 0;
	current_inode->creator_uid = 0;
	current_inode->size = file_size;

	fseek(fd, FILE_NAME_LIST_OFFSET, SEEK_SET);
	char *file_name_list_buff = (char *)malloc(BLOCK_SIZE);
	fread(file_name_list_buff, 1, BLOCK_SIZE, fd);

	for(i = 0; i < BLOCK_SIZE; ++i){
		if(file_name_list_buff[i] == 0 && file_name_list_buff[++i] == 0)
			break;
	}

	if(i == 1)
		--i;
	strcpy(file_name_list_buff + i, inserted_file_name);
	current_inode->name_address = i;
	fseek(fd, FILE_NAME_LIST_OFFSET, SEEK_SET);
	fwrite(file_name_list_buff, 1, BLOCK_SIZE, fd);

	fseek(fd, FREE_BLOCK_LIST_OFFEST, SEEK_SET);
	char *block_buff = (char *)malloc(BLOCK_SIZE);
	fread(block_buff, 1, BLOCK_SIZE, fd);
	
	int data_addresses_block_offset = find_free_blocks(block_buff, 1);
	current_inode->address_block = data_addresses_block_offset/SECTOR_SIZE;
	int *data_addresses_block_buff = (int *)malloc(BLOCK_SIZE);	
	int blocks_needed = (file_size - 1)/BLOCK_SIZE + 1;
	int first_data_block_offset = find_free_blocks(block_buff, blocks_needed);
	for(i = 0; i < blocks_needed; ++i)
		data_addresses_block_buff[i] = first_data_block_offset/SECTOR_SIZE + i*16;
	
	fseek(fd, data_addresses_block_offset, SEEK_SET);
	fwrite(data_addresses_block_buff, 1, BLOCK_SIZE, fd);

	FILE *file_fd = fopen(file_name, "r");
	char *file_data_buff = (char *)malloc(file_size);
	fread(file_data_buff, 1, file_size, file_fd);

	fseek(fd, first_data_block_offset, SEEK_SET);
	fwrite(file_data_buff, 1, file_size, fd);
	fclose(file_fd);

	fseek(fd, FREE_BLOCK_LIST_OFFEST, SEEK_SET);
	fwrite(block_buff, 1, BLOCK_SIZE, fd);

	fseek(fd, INODE_LIST_OFFSET, SEEK_SET);
	fwrite(inode_list_buff, 1, BLOCK_SIZE, fd);
	fclose(fd);
}
