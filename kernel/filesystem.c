#include "../headers/filesystem.h"
#include "../headers/inode.h"
#include "../headers/kernel.h"
#include "../headers/ata.h"
#include "../headers/heap.h"
#include "../headers/screen.h"
#include "../headers/string.h"
#include "../headers/list.h"
#include "../headers/process.h"

#define SECTOR_SIZE 512
#define BLOCK_SIZE 16
#define BOUND_BLOCKS_BITMAP 34816
#define INODE_LIST 34832
#define FILE_NAMES_LIST 34848
#define FILE_SYSTEM_BASE 34816

typedef struct {
	inode *inode;
	unsigned int inode_index;
	int *physical_address_block;
	list *file_data_blocks_list;
	int file_offset;
	int used;
} file_descriptor;

typedef struct {
	short index;
	char *data;
} file_data_block;

static int get_free_block();

static unsigned char *bound_blocks_bitmap;
static inode *inode_list;
static unsigned char *file_names_list;
static int inode_count = BLOCK_SIZE*SECTOR_SIZE/sizeof(inode);
static list *open_files_list;
static int bitmap_length = BLOCK_SIZE*SECTOR_SIZE;

void init_filesystem(void){ //initialize the file system structutes
	bound_blocks_bitmap = (unsigned char *)malloc(BLOCK_SIZE*SECTOR_SIZE*sizeof(unsigned char));
	ata_read_sectors(BOUND_BLOCKS_BITMAP, BLOCK_SIZE, bound_blocks_bitmap);			
	if(~(*bound_blocks_bitmap) & 0b111){
		print("initializing the bound block bitmap\n");
		*bound_blocks_bitmap = (*bound_blocks_bitmap) | 0b111;
		ata_write_sectors(BOUND_BLOCKS_BITMAP, BLOCK_SIZE, bound_blocks_bitmap);
	}

	inode_list = (inode *)malloc(BLOCK_SIZE*SECTOR_SIZE*sizeof(unsigned char));
	ata_read_sectors(INODE_LIST, BLOCK_SIZE, (char *)inode_list);
	
	file_names_list = (unsigned char *)malloc(BLOCK_SIZE*SECTOR_SIZE*sizeof(unsigned char));
	ata_read_sectors(FILE_NAMES_LIST, BLOCK_SIZE, file_names_list);
	open_files_list = create_list();
}

void get_inode(char *file_name, void *buff){ //copy the inode to the buffer
	int inode_index;
	for(inode_index = 0; inode_index<inode_count; ++inode_index){
		if((inode_list+inode_index)->bound && !strcmp((char *)((inode_list+inode_index)->name_address+file_names_list), file_name)){
			memcpy(buff, inode_list + inode_index, sizeof(inode));
			return;
		}
	}

	*(char *)buff = -1;
}

int get_file_size(char *file_name){ //return file size or -1 if doesnt exist
	int inode_index;
	for(inode_index = 0; inode_index<inode_count; ++inode_index){
		if((inode_list+inode_index)->bound && !strcmp((char *)((inode_list+inode_index)->name_address+file_names_list), file_name)){
			return (inode_list + inode_index)->size;
		}
	}

	return -1;
}

int get_inode_index_of(char *file_name){ 
	int *inode_index_list = (int *)malloc(BLOCK_SIZE*SECTOR_SIZE);
	char *current_path = strtok(file_name, "/");	
	int inode_index = 0;

	while(current_path){ //runs from the root directory to the file to find in, terminates when got the file or the directory/file don't exists
		ata_read_sectors(inode_list[inode_index].address_block, BLOCK_SIZE, (char *)inode_index_list);
		int i, length = BLOCK_SIZE*SECTOR_SIZE/sizeof(int);
		for(i = 0; i < length; ++i){
			if(inode_index_list[i] == 0)
				break;

			inode *current_inode = inode_list + inode_index_list[i];
			if(!strcmp(current_inode->name_address + file_names_list, current_path))
				break;
		}

		if(inode_index_list[i] == 0)
			break;
		else
			inode_index = inode_index_list[i];

		current_path = strtok(0, "/");
	}
	
	while(current_path){ //runs the rest of the path creating all directories/files needed
		int i, length = BLOCK_SIZE*SECTOR_SIZE/sizeof(inode);
		for(i = 0; i < length; ++i){
			if(!inode_list[i].bound)
				break;
		}

		inode *current_inode = inode_list + i;
		int _inode_index = i;
		for(i = 0; i < BLOCK_SIZE*SECTOR_SIZE; ++i){
			if(!strcmp(file_names_list + i, current_path))
				break;
			if(*(file_names_list + i) == '\0' && *(file_names_list + i + 1) == '\0'){
				++i;
				strcpy(file_names_list + i, current_path);
				ata_write_sectors(FILE_NAMES_LIST, BLOCK_SIZE, (char *)file_names_list);
				break;
			}
		}

		current_inode->name_address = i;
		current_inode->type = DIRECTORY;
		current_inode->bound = 1;
		current_inode->access = 0700;
		current_inode->creator_uid = get_current_euid();
		current_inode->size = 0;
		current_inode->address_block = get_free_block();
		current_inode->creation_date = 0;
		current_inode->update_date = 0;

		length = BLOCK_SIZE*SECTOR_SIZE/sizeof(int);
		for(i = 0; i < length; ++i){
			if(inode_index_list[i] == 0){
				inode_index_list[i] = _inode_index;
				break;
			}
		}
		ata_write_sectors(inode_list[inode_index].address_block, BLOCK_SIZE, (char *)inode_index_list);
		inode_index = _inode_index;

		current_path = strtok(0, "/");
		if(current_path){
			ata_read_sectors(current_inode->address_block, BLOCK_SIZE, (char *)inode_index_list);
		}else{
			current_inode->type = REGULAR_FILE;
			ata_write_sectors(INODE_LIST, BLOCK_SIZE, (char *)inode_list);
			break;
		}
	}

	free(inode_index_list);
	return inode_index;
}

FILE open(char *file_name){
	cli();
	print("opening ");
	print(file_name);
	print("\n");

	int inode_index = get_inode_index_of(file_name);
	list_node *current_file_descriptor_node = open_files_list->first;
	FILE file_descriptor_index = 0;
	while(current_file_descriptor_node){ //check if file already open
		file_descriptor *current_open_file = (file_descriptor *)current_file_descriptor_node->value;
		if(current_open_file->inode_index == inode_index && current_open_file->used)
			return file_descriptor_index;

		++file_descriptor_index;
		current_file_descriptor_node = current_file_descriptor_node->next;
	}
	
	current_file_descriptor_node = open_files_list->first;
	file_descriptor_index = 0;
	while(current_file_descriptor_node){ //getting the file descriptor index
		file_descriptor *current_file_descriptor = (file_descriptor *)current_file_descriptor_node->value; 
		if(!current_file_descriptor->used)
			break;

		++file_descriptor_index;
		current_file_descriptor_node = current_file_descriptor_node->next;
	}

	file_descriptor *file;
	if(!current_file_descriptor_node){ //creating if doesnt exist
		file = (file_descriptor *)malloc(sizeof(file_descriptor));	
		add_to_list(open_files_list, file);
	}else
		file = (file_descriptor *)current_file_descriptor_node->value;

	file->inode = inode_list+inode_index;
	file->inode_index = inode_index;
	file->physical_address_block = (int *)malloc(BLOCK_SIZE*SECTOR_SIZE);
	ata_read_sectors(file->inode->address_block, BLOCK_SIZE, (char *)file->physical_address_block);
	file->file_data_blocks_list = create_list();
	file->file_offset = 0;
	file->used = 1;
	sti();
	return file_descriptor_index; 
}

int get_free_block(void){ //get index of a free block and marks it as used
	int bitmap_index, mask;
	for(bitmap_index = 0; bitmap_index<bitmap_length; ++bitmap_index){
		for(mask = 1; mask<256; mask = mask<<1){
			if(!(*(bound_blocks_bitmap+bitmap_index) & mask)){
				int block_offset = 0;
				*(bound_blocks_bitmap+bitmap_index) = *(bound_blocks_bitmap+bitmap_index) | mask;
				ata_write_sectors(BOUND_BLOCKS_BITMAP, BLOCK_SIZE, bound_blocks_bitmap);
				while(mask != 1){
					++block_offset;
					mask = mask>>1;
				}
				return (bitmap_index*8 + block_offset)*BLOCK_SIZE + FILE_SYSTEM_BASE;
			}
		}
	}

	return -1;
}

int write(FILE file_descriptor_index, char *buff, int count){ //write to file
	file_descriptor *current_file_descriptor = (file_descriptor *)get_list_element(open_files_list, file_descriptor_index);	
	print("writing ");
	print(current_file_descriptor->inode->name_address + file_names_list);
	print("\n");
	int blocks_to_write = (current_file_descriptor->file_offset%(BLOCK_SIZE*SECTOR_SIZE) + count - 1)/(BLOCK_SIZE*SECTOR_SIZE) + 1;
	int block_index = current_file_descriptor->file_offset/(BLOCK_SIZE*SECTOR_SIZE);
	int data_offset = current_file_descriptor->file_offset%(BLOCK_SIZE*SECTOR_SIZE), buff_offset = 0;
	int temp_count = count;
	char end_of_file = 0;
	if(current_file_descriptor->file_offset + count >= current_file_descriptor->inode->size){
		++count;
		current_file_descriptor->inode->size = current_file_descriptor->file_offset + count;
		ata_write_sectors(INODE_LIST, BLOCK_SIZE, (char *)inode_list);
		end_of_file = 1;
	}
	
	for(block_index = 0; blocks_to_write--; ++block_index){
		list_node *current = current_file_descriptor->file_data_blocks_list->first;
		file_data_block *current_file_data_block = 0;
		while(current){
			if(((file_data_block *)current->value)->index == block_index){
				current_file_data_block = current->value;
				break;
			}

			current = current->next;
		}

		if(!current_file_data_block){
			set_vga_colors(WHITE, RED);
			print("data block not found\n");
			current_file_data_block = malloc(sizeof(file_data_block));
			current_file_data_block->index = block_index;

			if(!*(current_file_descriptor->physical_address_block+block_index)){
				set_vga_colors(WHITE, RED);
				print("data block not allocated\n");
				*(current_file_descriptor->physical_address_block+block_index) = get_free_block();
				ata_write_sectors(current_file_descriptor->inode->address_block, BLOCK_SIZE, (char *)current_file_descriptor->physical_address_block);
			}

			current_file_data_block->data = (char *)malloc(BLOCK_SIZE*SECTOR_SIZE*sizeof(char));
			ata_read_sectors(*(current_file_descriptor->physical_address_block+block_index), BLOCK_SIZE, current_file_data_block->data);			
			add_to_list(current_file_descriptor->file_data_blocks_list, current_file_data_block);
		}

		int bytes_to_write = (count - 1)%(BLOCK_SIZE*SECTOR_SIZE) + 1;
		memcpy(current_file_data_block->data+data_offset, buff+buff_offset, bytes_to_write);
		if(end_of_file && bytes_to_write == count)
			*(current_file_data_block->data+data_offset+count-1) = -1;
		count -= bytes_to_write;
		data_offset = 0;
		buff_offset += BLOCK_SIZE*SECTOR_SIZE;
		ata_write_sectors(*(current_file_descriptor->physical_address_block+block_index), BLOCK_SIZE, current_file_data_block->data);
	}

	current_file_descriptor->file_offset += temp_count;
	return temp_count;
}

int read(FILE file_descriptor_index, char *buff, int count){ //read from file
	file_descriptor *current_file_descriptor = (file_descriptor *)get_list_element(open_files_list, file_descriptor_index);	
	print("reading ");
	print(current_file_descriptor->inode->name_address + file_names_list);
	print("\n");
	if(count + current_file_descriptor->file_offset >= current_file_descriptor->inode->size)
		count = current_file_descriptor->inode->size - current_file_descriptor->file_offset - 1;
	int blocks_to_read = (current_file_descriptor->file_offset%(BLOCK_SIZE*SECTOR_SIZE) + count - 1)/(BLOCK_SIZE*SECTOR_SIZE) + 1;
	int block_index = current_file_descriptor->file_offset/(BLOCK_SIZE*SECTOR_SIZE);
	int data_offset = current_file_descriptor->file_offset%(BLOCK_SIZE*SECTOR_SIZE), buff_offset = 0;
	int temp_count = count;
	for(block_index = 0; blocks_to_read--; ++block_index){
		list_node *current = current_file_descriptor->file_data_blocks_list->first;
		file_data_block *current_file_data_block = 0;
		while(current){
			if(((file_data_block *)current->value)->index == block_index){
				current_file_data_block = current->value;
				break;
			}

			current = current->next;
		}

		if(!current_file_data_block){	
			set_vga_colors(WHITE, RED);
			print("data block not found\n");
			current_file_data_block = malloc(sizeof(file_data_block));
			current_file_data_block->index = block_index;

			if(!*(current_file_descriptor->physical_address_block+block_index)){
				*(buff+buff_offset) = -1;
				return;
			}
			current_file_data_block->data = (char *)malloc(BLOCK_SIZE*SECTOR_SIZE*sizeof(char));
			ata_read_sectors(*(current_file_descriptor->physical_address_block+block_index), BLOCK_SIZE, current_file_data_block->data);			
			add_to_list(current_file_descriptor->file_data_blocks_list, current_file_data_block);
		}

		int bytes_to_read = (count - 1)%(BLOCK_SIZE*SECTOR_SIZE) + 1;
		memcpy(buff+buff_offset, current_file_data_block->data+data_offset, bytes_to_read);
		count -= bytes_to_read;
		data_offset = 0;
		buff_offset += BLOCK_SIZE*SECTOR_SIZE;
	}

	current_file_descriptor->file_offset += temp_count;
	return temp_count;
}

void seek(FILE file_descriptor_index, int new_file_offset){ //seeks from beginning of a file
	((file_descriptor *)get_list_element(open_files_list, file_descriptor_index))->file_offset = new_file_offset;
}

void execute(char *file_name, int screen_index, int argc, char **argv){ //executes file
	cli();
	print("executing ");
	print(file_name);
	print("\n");
	FILE file_descriptor_index = open(file_name);
	file_descriptor *current_file_descriptor = (file_descriptor *)get_list_element(open_files_list, file_descriptor_index);	
	char *buff = (char *)malloc(current_file_descriptor->inode->size);
	current_file_descriptor->file_offset = 0;
	read(file_descriptor_index, buff, current_file_descriptor->inode->size);
	close(file_descriptor_index);
	create_process(buff, current_file_descriptor->inode->size, screen_index, file_name, argc, argv);
	sti();
}

void close(FILE file_descriptor_index){ //closes file
	file_descriptor *current_file_descriptor = (file_descriptor *)get_list_element(open_files_list, file_descriptor_index);	
	print("closing ");
	print(current_file_descriptor->inode->name_address + file_names_list);
	print("\n");
	free(current_file_descriptor->physical_address_block);

	list_node *current_file_data_block_node = current_file_descriptor->file_data_blocks_list->first;
	while(current_file_data_block_node){
		file_data_block *current_file_data_block = (file_data_block *)current_file_data_block_node->value;
		free(current_file_data_block->data);
		free(current_file_data_block);

		current_file_data_block_node = current_file_data_block_node->next;
	}

	free(current_file_descriptor->file_data_blocks_list);
	current_file_descriptor->used = 0;
}
