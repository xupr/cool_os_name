#pragma once
#include "list.h"

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
	DEVICE_TYPE device_type;
	unsigned short access;
	unsigned int creator_uid;
	unsigned int size;
	unsigned int address_block;
	unsigned short name_address;
	unsigned int creation_date;
	unsigned int update_date;
} inode;

struct stat{
	FILE_TYPE type;
	unsigned short access;
	unsigned int creator_uid;
	unsigned int size;
	unsigned int creation_date;
	unsigned int update_date;
};

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

typedef struct {
	inode *inode;
	unsigned int inode_index;
	int *inode_index_list;
	int used;
} dir_descriptor;

typedef struct {
	void (*open)(char *file_name, char *mode, file_descriptor *fd);
	int (*read)(char *buff, int count, file_descriptor *fd);
	int (*write)(char *buff, int count, file_descriptor *fd);
	void (*close)(file_descriptor *fd);
} special_file_methods;

typedef int FILE;
typedef int DIR;

void init_filesystem(void);
void add_special_file_method(DEVICE_TYPE d, special_file_methods *s);
inode *get_inode(char *file_name);
int get_file_size(char *file_name);
int stat(char *file_name, void *buff);
FILE open(char *file_name, char *mode);
DIR opendir(char *file_name);
int write(FILE file_descriptor_index, char *buff, int count);
int read(FILE file_descriptor_index, char *buff, int count);
char *readdir(DIR dir_descriptor_index, int index);
int execute(char *file_name, int screen_index, int argc, char **argv);
void seek(FILE file, int new_file_offset);
void close(FILE file_descriptor_index);
void closedir(DIR dir_descriptor_index);
