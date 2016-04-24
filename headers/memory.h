#include "../headers/process.h"

#define KERNEL_PAGE_TABLE 0

typedef struct {
	char present : 1;
	char read_write : 1;
	char user : 1;
	char write_through : 1;
	char cache_disable : 1;
	char accessed : 1;
	char zero : 1;
	char size : 1;
	char ignored : 1;
	char available : 3;
	int page_table : 20;
} page_directory_entry;

typedef struct {
	char present : 1;
	char read_write : 1;
	char user : 1;
	char write_through : 1;
	char cache_disable : 1;
	char accessed : 1;
	char dirty : 1;
	char zero : 1;
	char ignored : 1;
	char available : 3;
	int physical_page : 20;
} page_table_entry;

/*typedef struct {
	int page_table : 20;
	char available : 3;
	char ignored : 1;
	char size : 1;
	char zero : 1;
	char accessed : 1;
	char cache_disable : 1;
	char write_through : 1;
	char user : 1;
	char read_write : 1;
	char present : 1;
} page_directory_entry;

typedef struct {
	int physical_page : 20;
	char available : 3;
	char ignored : 1;
	char zero : 1;
	char dirty : 1;
	char accessed : 1;
	char cache_disable : 1;
	char write_through : 1;
	char user : 1;
	char read_write : 1;
	char present : 1;
} page_table_entry;*/
typedef unsigned int PAGE_TABLE;

void init_memory(char *memory_map_length, void *memory_map);
PAGE_TABLE create_page_table();
void switch_memory_map(PAGE_TABLE page_table_index);
void allocate_memory(PAGE_TABLE page_table_index, void *base, int limit);
void write_virtual_memory(PAGE_TABLE page_table_index, char *source, void *base, int limit);
void identity_page(PAGE_TABLE page_table_index, void *base, int limit);
void free_page_table(PAGE_TABLE page_table_index);
void dump_memory_map(void);
