#include "../headers/filesystem.h"
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

static int get_free_block();

static unsigned char *bound_blocks_bitmap;
static inode *inode_list;
static unsigned char *file_names_list;
static int inode_count = BLOCK_SIZE*SECTOR_SIZE/sizeof(inode);
static list *open_files_list;
static list *open_dirs_list;
static int bitmap_length = BLOCK_SIZE*SECTOR_SIZE;
static special_file_methods *sf_methods[NUM_DEVICE_TYPES];

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
	open_dirs_list = create_list();
}

inode *get_inode(char *file_name){ //copy the inode to the buffer
	int inode_index = get_inode_index_of(file_name, 0);
	if(inode_index == -1)
		return 0;
	
	return inode_list + inode_index;
}

void add_special_file_method(DEVICE_TYPE d, special_file_methods *s){
	sf_methods[d] = s;
}

int get_file_size(char *file_name){ //return file size or -1 if doesnt exist
	int inode_index = get_inode_index_of(file_name, 0);
	if(inode_index < 0)
		return -1;

	return (inode_list + inode_index)->size;
}

int request_permission(inode *current_inode, char *requested_permissions){
	int euid = get_current_euid();
	if(euid == 0)
		return 1;

	short relevent_access_bits;
	if(euid == current_inode->creator_uid)
		relevent_access_bits = current_inode->access>>6;
	else
		relevent_access_bits = current_inode->access&7;

	if(!strcmp(requested_permissions, "r") && relevent_access_bits&4)
		return 1;
	if(!strcmp(requested_permissions, "w") && relevent_access_bits&2)
		return 1;
	if(!strcmp(requested_permissions, "r+") && relevent_access_bits&4 && relevent_access_bits&2)
		return 1;
	if(!strcmp(requested_permissions, "x") && relevent_access_bits&1)
		return 1;

	return 0;
}

int get_inode_index_of(char *file_name, int create_if_missing){ 
	cli();
	if(!strcmp(file_name, "/"))
		return 0;
	if(file_name[0] == '/')
		++file_name;
	if(file_name[strlen(file_name) - 1] == '/')
		file_name[strlen(file_name) - 1] = '\0';

	char *_file_name = malloc(strlen(file_name) + 1);
	strcpy(_file_name, file_name);
	file_name = _file_name;
	int *inode_index_list = (int *)malloc(BLOCK_SIZE*SECTOR_SIZE);
	char *current_path = strtok(file_name, "/");	
	char *next_path, *previous_path = 0;
	int inode_index = 0;

	while(current_path){ //runs from the root directory to the file to find in, terminates when got the file or the directory/file don't exists
		next_path = strtok(0, "/");
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

		if(next_path && !request_permission(inode_list + inode_index, "x")){
			set_vga_colors(WHITE, RED);
			print("no permissions to enter ");
			set_vga_colors(WHITE, RED);
			print(current_path);
			print("\n");
			free(inode_index_list);
			free(file_name);
			sti();
			return -2;
		}

		previous_path = current_path;
		current_path = next_path;
	}

	if(!current_path){
		free(inode_index_list);
		free(file_name);
		sti();
		return inode_index;
	}

	if(!create_if_missing){
		free(inode_index_list);
		free(file_name);
		sti();
		return -1;
	}

	if(!request_permission(inode_list + inode_index, "w")){
		set_vga_colors(WHITE, RED);
		print("no permission to write in ");
		set_vga_colors(WHITE, RED);
		if(previous_path)
			print(previous_path);
		else 
			print("/");
		print("\n");
		free(inode_index_list);
		free(file_name);
		sti();
		return -2;
	}
	
	while(current_path){ //runs the rest of the path creating all directories/files needed
		print(current_path);
		print(" doesn't exist, creating\n");
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
		current_inode->major = NOT_SPECIAL;
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

		previous_path = current_path;
		current_path = next_path;
		next_path = strtok(0, "/");
		if(current_path){
			ata_read_sectors(current_inode->address_block, BLOCK_SIZE, (char *)inode_index_list);
		}else{
			current_inode->type = NEW_FILE;
			ata_write_sectors(INODE_LIST, BLOCK_SIZE, (char *)inode_list);
			free(inode_index_list);
			free(file_name);
			sti();
			return inode_index;
		}
	}
}

int stat(char *file_name, void *buff){
	int inode_index = get_inode_index_of(file_name, 0);	
	if(inode_index < 0)
		return -1;

	inode *current_inode = inode_list + inode_index;
	struct stat *st = (struct stat *)buff;
	st->type = current_inode->type;
	st->access = current_inode->access;
	st->creator_uid = current_inode->creator_uid;
	st->size = current_inode->size;
	st->creation_date = current_inode->creation_date;
	st->update_date = current_inode->update_date;
	return 0;
}

FILE open(char *file_name, char *mode){
	cli();
	print("opening ");
	print(file_name);
	print("\n");

	int inode_index = get_inode_index_of(file_name, 1);
	if(inode_list[inode_index].type == NEW_FILE)
		inode_list[inode_index].type = REGULAR_FILE;

	if(inode_index < 0 || !request_permission(inode_list + inode_index, mode) || !(inode_list[inode_index].type == REGULAR_FILE || inode_list[inode_index].type == SPECIAL_FILE)){
		sti();
		return -1;
	}

	list_node *current_file_descriptor_node = open_files_list->first;
	FILE file_descriptor_index = 0;
	while(current_file_descriptor_node){ //check if file already open
		file_descriptor *current_open_file = (file_descriptor *)current_file_descriptor_node->value;
		if(current_open_file->inode_index == inode_index && current_open_file->used){
			if(inode_list[inode_index].type == SPECIAL_FILE && sf_methods[inode_list[inode_index].major]->open != 0)
				sf_methods[inode_list[inode_index].major]->open(file_name, mode, current_open_file);
			sti();
			return file_descriptor_index;
		}
		
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
	if(inode_list[inode_index].type == SPECIAL_FILE && sf_methods[inode_list[inode_index].major]->open != 0)
		sf_methods[inode_list[inode_index].major]->open(file_name, mode, file);
	sti();
	return file_descriptor_index; 
}

DIR opendir(char *file_name){
	cli();
	print("opening ");
	print(file_name);
	print("\n");

	int inode_index = get_inode_index_of(file_name, 0);
	if(inode_list[inode_index].type == NEW_FILE)
		inode_list[inode_index].type = DIRECTORY;

	if(inode_index < 0 || !request_permission(inode_list + inode_index, "r") || inode_list[inode_index].type != DIRECTORY)
		return -1;

	list_node *current_dir_descriptor_node = open_dirs_list->first;
	DIR dir_descriptor_index = 0;
	while(current_dir_descriptor_node){ //check if file already open
		dir_descriptor *current_open_dir = (dir_descriptor *)current_dir_descriptor_node->value;
		if(current_open_dir->inode_index == inode_index && current_open_dir->used)
			return dir_descriptor_index;

		++dir_descriptor_index;
		current_dir_descriptor_node = current_dir_descriptor_node->next;
	}
	
	current_dir_descriptor_node = open_dirs_list->first;
	dir_descriptor_index = 0;
	while(current_dir_descriptor_node){ //getting the file descriptor index
		dir_descriptor *current_dir_descriptor = (dir_descriptor *)current_dir_descriptor_node->value; 
		if(!current_dir_descriptor->used)
			break;

		++dir_descriptor_index;
		current_dir_descriptor_node = current_dir_descriptor_node->next;
	}

	dir_descriptor *dir;
	if(!current_dir_descriptor_node){ //creating if doesnt exist
		dir = (dir_descriptor *)malloc(sizeof(dir_descriptor));	
		add_to_list(open_dirs_list, dir);
	}else
		dir = (dir_descriptor *)current_dir_descriptor_node->value;

	dir->inode = inode_list+inode_index;
	dir->inode_index = inode_index;
	dir->inode_index_list = (int *)malloc(BLOCK_SIZE*SECTOR_SIZE);
	ata_read_sectors(dir->inode->address_block, BLOCK_SIZE, (char *)dir->inode_index_list);
	dir->used = 1;
	sti();
	return dir_descriptor_index; 
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
	if(!current_file_descriptor)
		return 0;

	if(current_file_descriptor->inode->type == SPECIAL_FILE)
		if(sf_methods[current_file_descriptor->inode->major]->write != 0)
			return sf_methods[current_file_descriptor->inode->major]->write(buff, count, current_file_descriptor);
		else
			return -1;

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
	if(!current_file_descriptor)
		return 0;

	if(current_file_descriptor->inode->type == SPECIAL_FILE)
		if(sf_methods[current_file_descriptor->inode->major]->read != 0)
			return sf_methods[current_file_descriptor->inode->major]->read(buff, count, current_file_descriptor);
		else
			return -1;

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

char *readdir(DIR dir_descriptor_index, int index){
	int inode_index = ((dir_descriptor *)get_list_element(open_dirs_list, dir_descriptor_index))->inode_index_list[index];
	if(!inode_index)
		return 0;

	return file_names_list + inode_list[inode_index].name_address;
}

void seek(FILE file_descriptor_index, int new_file_offset){ //seeks from beginning of a file
	file_descriptor *current_file_descriptor = (file_descriptor *)get_list_element(open_files_list, file_descriptor_index);
	if(!current_file_descriptor)
		return;

	if(current_file_descriptor->inode->type == SPECIAL_FILE){
		if(sf_methods[current_file_descriptor->inode->major]->seek != 0)
			sf_methods[current_file_descriptor->inode->major]->seek(current_file_descriptor, new_file_offset);
	}else
		current_file_descriptor->file_offset = new_file_offset;
}

int execute(char *file_name, FILE stdin, FILE stdout, int argc, char **argv){ //executes file
	cli();
	print("executing ");
	print(file_name);
	print("\n");
	FILE file_descriptor_index = open(file_name, "x");
	if(file_descriptor_index == -1){
		sti();
		return -1;
	}
	file_descriptor *current_file_descriptor = (file_descriptor *)get_list_element(open_files_list, file_descriptor_index);	
	if(!current_file_descriptor)
		return 0;

	char *buff = (char *)malloc(current_file_descriptor->inode->size);
	current_file_descriptor->file_offset = 0;
	read(file_descriptor_index, buff, current_file_descriptor->inode->size);
	close(file_descriptor_index);
	create_process(buff, current_file_descriptor->inode->size, stdin, stdout, file_name, argc, argv);
	sti();
	return 0;
}

void close(FILE file_descriptor_index){ //closes file
	cli();
	file_descriptor *current_file_descriptor = (file_descriptor *)get_list_element(open_files_list, file_descriptor_index);	
	if(!current_file_descriptor)
		return;

	print("closing ");
	print(current_file_descriptor->inode->name_address + file_names_list);
	print("\n");
	if(current_file_descriptor->inode->type == SPECIAL_FILE && sf_methods[current_file_descriptor->inode->major]->close != 0)
		sf_methods[current_file_descriptor->inode->major]->close(current_file_descriptor);

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
	sti();
}

void closedir(DIR dir_descriptor_index){ //closes file
	cli();
	dir_descriptor *current_dir_descriptor = (dir_descriptor *)get_list_element(open_dirs_list, dir_descriptor_index);	
	if(!current_dir_descriptor)
		return;

	print("closing ");
	print(current_dir_descriptor->inode->name_address + file_names_list);
	print("\n");
	free(current_dir_descriptor->inode_index_list);
	current_dir_descriptor->used = 0;
	sti();
}
