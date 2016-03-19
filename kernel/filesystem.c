#include "../headers/filesystem.h"
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
	unsigned char bound;
	unsigned short access;
	unsigned int size;
	unsigned int address_block;
	unsigned short name_address;
	unsigned int creation_date;
	unsigned int update_date;
} inode;

typedef struct {
	inode *inode;
	int *physical_address_block;
	list *file_data_blocks_list;
	int file_offset;
} file_descriptor;

typedef struct {
	short index;
	char *data;
} file_data_block;

static int get_free_block();

/*
disk memory structure:
[bootloader|kernel  |bound blocks bitmap|inode block|file names block|files.......]
[--512b----|1mb-512b|------1block-------|--1block---|------1block----|rest-of-disk]
specs:
block size = 16 sectors = 8kb
inode size = 24b
file name limit = 23 chars
file size limit = 16mb
files limit = 341 files
max disk size = 5.3gb
*/

static unsigned char *bound_blocks_bitmap;
static inode *inode_list;
static unsigned char *file_names_list;
static int inode_count = BLOCK_SIZE*SECTOR_SIZE/sizeof(inode);
static list *open_files_list;
static int bitmap_length = BLOCK_SIZE*SECTOR_SIZE;

void init_filesystem(void){
	bound_blocks_bitmap = (unsigned char *)malloc(BLOCK_SIZE*SECTOR_SIZE*sizeof(unsigned char));
	ata_read_sectors(BOUND_BLOCKS_BITMAP, BLOCK_SIZE, bound_blocks_bitmap);			
	if(~(*bound_blocks_bitmap) & 0b111){
		print("initializing the bound block bitmap\n");
		//print(itoa(*bound_blocks_bitmap));
		*bound_blocks_bitmap = (*bound_blocks_bitmap) | 0b111;
		//print(itoa(*bound_blocks_bitmap));
		//int i;
		//for(i = 0; i < 272; *(bound_blocks_bitmap+i++) = 0xFF);
		ata_write_sectors(BOUND_BLOCKS_BITMAP, BLOCK_SIZE, bound_blocks_bitmap);
	}

	inode_list = (inode *)malloc(BLOCK_SIZE*SECTOR_SIZE*sizeof(unsigned char));
	ata_read_sectors(INODE_LIST, BLOCK_SIZE, (char *)inode_list);
	
	file_names_list = (unsigned char *)malloc(BLOCK_SIZE*SECTOR_SIZE*sizeof(unsigned char));
	ata_read_sectors(FILE_NAMES_LIST, BLOCK_SIZE, file_names_list);
	open_files_list = create_list();
}

FILE open(char *file_name){
	int inode_index;
//	print("\n");
	print("opening ");
	print(file_name);
	print("\n");
	list_node *current_file = open_files_list->first;
	int index = 0;
	while(current_file){
		if(!strcmp(((file_descriptor *)current_file->value)->inode->name_address + file_names_list, file_name)){
			print("file already open\n");
			return index;
		}
		current_file = current_file->next;
		++index;
	}

	for(inode_index = 0; inode_index<inode_count; ++inode_index){
		if((inode_list+inode_index)->bound && !strcmp((char *)((inode_list+inode_index)->name_address+file_names_list), file_name)){
			//print(itoa((inode_list+inode_index)->name_address));
			//print("\n");
			//print(((inode_list+inode_index)->name_address+file_names_list));
			file_descriptor *file = (file_descriptor *)malloc(sizeof(file_descriptor));
			file->inode = (inode *)(inode_list+inode_index);
			file->physical_address_block = (int *)malloc(BLOCK_SIZE*SECTOR_SIZE);
			//print(itoa(file->inode->address_block));
			ata_read_sectors(file->inode->address_block, BLOCK_SIZE, (char *)file->physical_address_block);
			file->file_data_blocks_list = create_list();
			file->file_offset = 0;
			add_to_list(open_files_list, file);
			return (FILE)(open_files_list->length-1);	
		}
	}
	
	//print("1");
	for(inode_index = 0; inode_index<inode_count; ++inode_index){
		if(!(inode_list+inode_index)->bound){
			inode *file_node = inode_list+inode_index;
			file_node->bound = 1;
			file_node->access = 0700;
			file_node->size = 0;
			file_node->address_block = get_free_block();
			int name_address = 0;
			//print("2");
			//while(strcmp(file_names_list+name_address++, "\0"));
			while(*(file_names_list + name_address++) || *(file_names_list + name_address));
			//print(itoa(name_address));
			//print("3");
			if(name_address == 1)
				--name_address;
			file_node->name_address = name_address;
			strcpy(file_names_list+name_address, file_name);
			ata_write_sectors(FILE_NAMES_LIST, BLOCK_SIZE, file_names_list);
			file_node->creation_date = 0;
			file_node->update_date = 0;
			ata_write_sectors(INODE_LIST, BLOCK_SIZE, (char *)inode_list);
			//print("4");

			file_descriptor *file = (file_descriptor *)malloc(sizeof(file_descriptor));
			//print("5");
			file->inode = file_node;
			file->physical_address_block = (int *)malloc(BLOCK_SIZE*SECTOR_SIZE*sizeof(char));
			//print(itoa(file->physical_address_block));
			ata_read_sectors(file_node->address_block, BLOCK_SIZE, (char *)file->physical_address_block);
			file->file_data_blocks_list = create_list();
			file->file_offset = 0;
			//print("6");
			add_to_list(open_files_list, file);
			//print("7");
			return (FILE)(open_files_list->length-1);
		}
	}
	
	return -1;
}

int get_free_block(void){
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

int write(FILE file_descriptor_index, char *buff, int count){
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
	
	//print("\n");
	//print(buff);
	//print(itoa(block_index));
	//print("\n");
	//print(itoa(blocks_to_write));
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
				//print(itoa((current_file_descriptor->inode->address_block)));
				ata_write_sectors(current_file_descriptor->inode->address_block, BLOCK_SIZE, (char *)current_file_descriptor->physical_address_block);
			}

			current_file_data_block->data = (char *)malloc(BLOCK_SIZE*SECTOR_SIZE*sizeof(char));
			ata_read_sectors(*(current_file_descriptor->physical_address_block+block_index), BLOCK_SIZE, current_file_data_block->data);			
			add_to_list(current_file_descriptor->file_data_blocks_list, current_file_data_block);
		}

		int bytes_to_write = (count - 1)%(BLOCK_SIZE*SECTOR_SIZE) + 1;
		//print(itoa(current_file_data_block->data+data_offset));
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

int read(FILE file_descriptor_index, char *buff, int count){	
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
	//print(itoa(blocks_to_read));
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
/*				print("data block not allocated\n");
				*(current_file_descriptor->physical_address_block+block_index) = get_free_block();
				print(itoa((current_file_descriptor->inode->address_block)*BLOCK_SIZE));
				ata_write_sectors(current_file_descriptor->inode->address_block, BLOCK_SIZE, (char *)current_file_descriptor->physical_address_block);
*/			}
			//print(itoa(*(current_file_descriptor->physical_address_block)));
			current_file_data_block->data = (char *)malloc(BLOCK_SIZE*SECTOR_SIZE*sizeof(char));
			ata_read_sectors(*(current_file_descriptor->physical_address_block+block_index), BLOCK_SIZE, current_file_data_block->data);			
			add_to_list(current_file_descriptor->file_data_blocks_list, current_file_data_block);
		}

		int bytes_to_read = (count - 1)%(BLOCK_SIZE*SECTOR_SIZE) + 1;
		//print(itoa(current_file_data_block->data+data_offset));
		memcpy(buff+buff_offset, current_file_data_block->data+data_offset, bytes_to_read);
		count -= bytes_to_read;
		data_offset = 0;
		buff_offset += BLOCK_SIZE*SECTOR_SIZE;
	}

	current_file_descriptor->file_offset += temp_count;
	return temp_count;
}

void seek(FILE file_descriptor_index, int new_file_offset){
	((file_descriptor *)get_list_element(open_files_list, file_descriptor_index))->file_offset = new_file_offset;
}

void execute(char *file_name){
	print("executing ");
	print(file_name);
	print("\n");
	FILE file_descriptor_index = open(file_name);
	file_descriptor *current_file_descriptor = (file_descriptor *)get_list_element(open_files_list, file_descriptor_index);	
	//print(itoa(current_file_descriptor->inode->size));
	char *buff = (char *)malloc(current_file_descriptor->inode->size);
	read(file_descriptor_index, buff, current_file_descriptor->inode->size);
	create_process(buff, current_file_descriptor->inode->size);
//	asm("call eax" : : "a"(buff));
}
