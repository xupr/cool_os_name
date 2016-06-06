#include "../headers/memory.h"
#include "../headers/kernel.h"
#include "../headers/screen.h"
#include "../headers/string.h"
#include "../headers/list.h"
#include "../headers/heap.h"
#include "../headers/process.h"
#include "../headers/stdlib.h"

#define PAGE_SIZE 0x1000
#define KERNEL_BASE 0x100000
#define KERNEL_LIMIT 0x500000

typedef enum {
	USABLE = 1,
	RESERVED = 2,
	ACPI_RECLAIMABLE = 3,
	ACPI_NVS = 4,
	BAD = 5
} memory_block_type;

typedef struct {
	void *base;
	int zero1;
	int limit;
	int zero2;
	memory_block_type type;
} memory_information_block;

typedef struct {
	char bound;
	char pinned;
	void *base;
	int limit;
	PAGE_TABLE page_table;
} memory_page_block;

typedef struct {
	int bound;
	page_directory_entry *page_table;
} page_table_descriptor;

static page_directory_entry kernel_page_directory[1024]__attribute__((aligned(PAGE_SIZE))); //kernel page directories
static page_table_entry kernel_page_table[8*1024]__attribute__((aligned(PAGE_SIZE))); //kernel page tables
static list *page_table_list;
static list *memory_page_map;

static PAGE_TABLE kernel_page_table_index = 0;

void dump_memory_map(void){ //prints the memory map
	print("\n");
	set_tab_size(12);
	print("page table\tbase\tlimit\n");
	list_node *current_memory_page_block_node = memory_page_map->first;
	while(current_memory_page_block_node){
		memory_page_block *current_memory_page_block = (memory_page_block *)current_memory_page_block_node->value;
		print(itoa(current_memory_page_block->page_table));
		print("\t");
		print(itoa((int)current_memory_page_block->base));
		print("\t");
		print(itoa(current_memory_page_block->limit));
		print("\n");

		current_memory_page_block_node = current_memory_page_block_node->next;
	}
	set_tab_size(8);
	print("\n");
}

static int compare_memory_blocks(void *a, void *b){ //compares two memory blocks for sorting
	unsigned int c = (unsigned int)(((memory_information_block *)a)->base), d = (unsigned int)(((memory_information_block *)b)->base);
	if(c > d)
		return 1;
	return -2;
}

void init_memory(char *memory_map_length, void *memory_map){ //initialize memory and it structures 
	memory_page_map = create_list(); //initialize the memory map
	int i, j;
	sort(memory_map, sizeof(memory_information_block), *memory_map_length, compare_memory_blocks);
	for(i = 0; i < *memory_map_length; ++i){ 
		memory_information_block *current_block = (memory_information_block *)memory_map+i;
		memory_page_block *page_block = (memory_page_block *)malloc(sizeof(memory_page_block));
		if(current_block->type == USABLE && (int)current_block->base >= 0x100000){
			if((int)current_block->base >= 0x100000 && (int)current_block->base < 0x600000){
				memory_page_block *kernel_page_block = (memory_page_block *)malloc(sizeof(memory_page_block));
				kernel_page_block->bound = 1;
				kernel_page_block->pinned = 1;
				kernel_page_block->base = (void *)0x100000;
				kernel_page_block->limit = 0x4FFFFF;
				kernel_page_block->page_table = KERNEL_PAGE_TABLE;

				page_block->bound = 0;
				page_block->pinned = 0;
				page_block->base = (void *)0x600000;
				page_block->limit = (int)current_block->base + current_block->limit - 0x600000 - 1;
				page_block->page_table = KERNEL_PAGE_TABLE;
			}else{
				page_block->bound = 0;
				page_block->pinned = 0;
				page_block->base = current_block->base;
				page_block->limit = current_block->limit;
				page_block->page_table = KERNEL_PAGE_TABLE;
			}

		}else{
			page_block->bound = 1;
			page_block->pinned = 1;
			page_block->base = current_block->base;
			page_block->limit = current_block->limit;
			page_block->page_table = KERNEL_PAGE_TABLE;
		}

		add_to_list(memory_page_map, page_block);
	}
	
	print("--memory map inititalized\n");
	page_table_list = create_list(); //initialize kernel page table
	PAGE_TABLE os_page_table_index = create_page_table();
	identity_page(os_page_table_index, (void *)0, 32*1024*1024-1);
	switch_memory_map(kernel_page_table_index);
	asm("mov eax, cr0; or eax, 0x80000000; mov cr0, eax"); //enable paging
	print("--paging initiailzed\n");
}

PAGE_TABLE create_page_table(){ //create a new page table
	cli();
	list_node *current = page_table_list->first;
	char found = 0;
	PAGE_TABLE page_table_index = 0;
	page_table_descriptor *current_page_table_descriptor;
	while(current != 0){ //found a not needed page table
		if(!((page_table_descriptor *)current->value)->bound){
			((page_table_descriptor *)current->value)->bound = 1;
			sti();
			return (PAGE_TABLE)page_table_index;
		}
		
		++page_table_index;
		current = current->next;
	}
	
	if(!found){ //if not found create one
		current_page_table_descriptor = (page_table_descriptor *)malloc(sizeof(page_table_descriptor));
		current_page_table_descriptor->page_table = (page_directory_entry *)malloc(2048*sizeof(page_directory_entry));
		page_directory_entry *page_table_address = (page_directory_entry *)((int)current_page_table_descriptor->page_table & (int)(~0xFFF));
		if(page_table_address < current_page_table_descriptor->page_table)
			page_table_address = (page_directory_entry *)((char *)page_table_address + PAGE_SIZE);
		current_page_table_descriptor->page_table = page_table_address;
		current_page_table_descriptor->bound = 1;
		add_to_list(page_table_list, current_page_table_descriptor);
	}

	int i; //zero the page table out
	page_directory_entry current_page_directory;
	current_page_directory.ignored = 1;
	current_page_directory.present = 0;
	for(i = 0; i < 1024; current_page_table_descriptor->page_table[i++] = current_page_directory);
	sti();
	return page_table_index; 
}

void switch_memory_map(PAGE_TABLE page_table_index){ //switch to a different page table
	cli();
	page_table_descriptor *current_page_table_descriptor = get_list_element(page_table_list, (int)page_table_index);
	asm("mov cr3, eax" : : "a"(current_page_table_descriptor->page_table));
	sti();
}

void allocate_memory(PAGE_TABLE page_table_index, void *base, int limit){ //alocate memory for a page table
	cli();
	int pages = (limit + ((int)base)%PAGE_SIZE - 1)/PAGE_SIZE + 1;
	int page_directories = (limit + ((int)base)%0x400000 - 1)/0x400000 + 1;
	int i;
	int current_pages = 1024 - (((int)base>>12) & 0x3FF);
	page_directory_entry *current_page_directory = (((page_table_descriptor *)get_list_element(page_table_list, page_table_index))->page_table + ((int)base>>22));
	for(i = 0; i++ < page_directories; ++current_page_directory){
		page_table_entry *current_page_table;
		if(current_page_directory->ignored){ //if a page directory does not exist, create one
			current_page_table = (page_table_entry *)malloc(2048*sizeof(page_table_entry));
			if((int)current_page_table & (int)(~0xFFF) > (int)current_page_table)
				current_page_table = (page_table_entry *)((int)current_page_table & (int)(~0xFFF));
			else
				current_page_table = (page_table_entry *)(((int)current_page_table & (int)(~0xFFF)) + PAGE_SIZE);
			
			current_page_directory->page_table = ((int)current_page_table)>>12;
			current_page_directory->available = 0;
			current_page_directory->ignored = 0;
			current_page_directory->size = 0;
			current_page_directory->zero = 0;
			current_page_directory->accessed = 0;
			current_page_directory->cache_disable = 0;
			current_page_directory->write_through = 1;
			current_page_directory->user = 1;
			current_page_directory->read_write = 1;
			current_page_directory->present = 1;
		
			int i;
			page_table_entry temp_page_table;
			temp_page_table.ignored = 1;
			temp_page_table.present = 0;
			temp_page_table.physical_page = 0;
			for(i = 0; i < 1024; current_page_table[i++] = temp_page_table);
		}
		current_page_table = ((page_table_entry *)(current_page_directory->page_table<<12) + (((int)base>>12) & 0x3FF));
		int j;
		for(j = 0; j++ < current_pages && pages--; ++current_page_table){
			if(current_page_table->ignored){ //if a page table does not exist, create one
				list_node *current_memory_page_block = memory_page_map->first;
				int index = 0;
				while(current_memory_page_block){ //find a useable memory block
					if(!((memory_page_block *)current_memory_page_block->value)->bound && \
					(int)((memory_page_block *)current_memory_page_block->value)->base > 0x100000)
						break;

					++index;
					current_memory_page_block = current_memory_page_block->next;
				}
				
				if(index < memory_page_map->length){
					void *page_address; //update the memory map
					if(index != memory_page_map->length - 1 &&\
					((memory_page_block *)(current_memory_page_block->next->value))->page_table == page_table_index){
						((memory_page_block *)current_memory_page_block->next->value)->base -= PAGE_SIZE;
						page_address = ((memory_page_block *)current_memory_page_block->next->value)->base;
						((memory_page_block *)current_memory_page_block->next->value)->limit += PAGE_SIZE;
						((memory_page_block *)current_memory_page_block->value)->limit -= PAGE_SIZE;
					}else if(index != 0 &&\
					((memory_page_block *)get_list_element(memory_page_map, index - 1))->page_table == page_table_index){
						((memory_page_block *)get_list_element(memory_page_map, index - 1))->limit += PAGE_SIZE;
						page_address = ((memory_page_block *)current_memory_page_block->value)->base; 
						((memory_page_block *)current_memory_page_block->value)->base += PAGE_SIZE;
						((memory_page_block *)current_memory_page_block->value)->limit -= PAGE_SIZE;
					}else{
						memory_page_block *temp_page_block = (memory_page_block *)malloc(sizeof(memory_page_block));
						temp_page_block->bound = 1;
						temp_page_block->pinned = 0;
						temp_page_block->base = ((memory_page_block *)current_memory_page_block->value)->base;
						temp_page_block->limit = PAGE_SIZE;
						temp_page_block->page_table = page_table_index;
						add_to_list_at(memory_page_map, temp_page_block, index);
						page_address = temp_page_block->base;
						((memory_page_block *)current_memory_page_block->value)->base += PAGE_SIZE;
						((memory_page_block *)current_memory_page_block->value)->limit -= PAGE_SIZE;
					}

				
					if(((memory_page_block *)current_memory_page_block->value)->limit <= 0){ //clean up if needed
						remove_from_list(memory_page_map, current_memory_page_block->value);
						free(current_memory_page_block);
					}

					current_page_table->physical_page = (int)page_address>>12; //initialize the page table
					current_page_table->available = 0;
					current_page_table->ignored = 0;
					current_page_table->zero = 0;
					current_page_table->dirty = 0;
					current_page_table->accessed = 0;
					current_page_table->cache_disable = 0;
					current_page_table->write_through = 1;
					current_page_table->user = 1;
					current_page_table->read_write = 1;
					current_page_table->present = 1;
				}
			}
		}

		current_pages = 1024;
		base = (void *)0;
	}
	sti();
}

void write_virtual_memory(PAGE_TABLE page_table_index, char *source, void *base, int limit){ //write into virtual memory	
	cli();
	int pages = (limit + ((int)base)%PAGE_SIZE - 1)/PAGE_SIZE + 1;
	int page_directories = (limit + ((int)base)%0x400000 - 1)/0x400000 + 1;
	page_directory_entry *current_page_directory = ((page_table_descriptor *)get_list_element(page_table_list, page_table_index))->page_table + ((int)base>>22);
	int i;
	int current_pages = 1024 - (((int)base>>12) & 0x3FF);
	for(i = 0; i++ < page_directories; ++current_page_directory){
		page_table_entry *current_page_table;
		current_page_table = ((page_table_entry *)(current_page_directory->page_table<<12)) + (((int)base>>12) & 0x3FF);
		int j;
		for(j = 0; j++ < current_pages && pages--; ++current_page_table){
			int length;
			if(limit >= PAGE_SIZE)
				length = PAGE_SIZE;
			else
				length = limit;
			memcpy((char *)(((current_page_table->physical_page)<<12) + (int)base%0x1000), source, length);
			limit -= PAGE_SIZE;
			source += PAGE_SIZE;
		}
		
		base = (void *)0;
		current_pages = 1024;
	}
	sti();
}

void identity_page(PAGE_TABLE page_table_index, void *base, int limit){ //same as allocate_memory but insted of allocating from free space, allocates so that the physical address matches the virtual
	cli();
	int pages = (limit + ((int)base)%PAGE_SIZE - 1)/PAGE_SIZE + 1;
	int page_directories = (limit + ((int)base)%0x400000 - 1)/0x400000 + 1;
	int i;
	int current_pages = 1024 - (((int)base>>12) & 0x3FF);
	void *current_page = base;
	page_directory_entry *current_page_directory = ((page_table_descriptor *)get_list_element(page_table_list, page_table_index))->page_table + ((int)base>>22);
	for(i = 0; i++ < page_directories; ++current_page_directory){
		page_table_entry *current_page_table;
		if(current_page_directory->ignored){
			current_page_table = (page_table_entry *)malloc(2048*sizeof(page_table_entry));
			if((int)current_page_table & (int)(~0xFFF) > (int)current_page_table)
				current_page_table = (page_table_entry *)((int)current_page_table & (int)(~0xFFF));
			else
				current_page_table = (page_table_entry *)(((int)current_page_table & (int)(~0xFFF)) + PAGE_SIZE);
			
			current_page_directory->page_table = ((int)current_page_table)>>12;
			current_page_directory->available = 0;
			current_page_directory->ignored = 0;
			current_page_directory->size = 0;
			current_page_directory->zero = 0;
			current_page_directory->accessed = 0;
			current_page_directory->cache_disable = 0;
			current_page_directory->write_through = 1;
			current_page_directory->user = 1;
			current_page_directory->read_write = 1;
			current_page_directory->present = 1;
		
			int i;
			page_table_entry temp_page_table;
			temp_page_table.ignored = 1;
			temp_page_table.present = 0;
			temp_page_table.physical_page = 0;
			for(i = 0; i < 1024; current_page_table[i++] = temp_page_table);
		}
		current_page_table = ((page_table_entry *)(current_page_directory->page_table<<12)) + (((int)base>>12) & 0x3FF);
		int j;
		for(j = 0; j++ < current_pages && pages--; ++current_page_table){
			if(current_page_table->ignored){
				current_page_table->physical_page = (int)current_page>>12;
				current_page_table->available = 0;
				current_page_table->ignored = 0;
				current_page_table->zero = 0;
				current_page_table->dirty = 0;
				current_page_table->accessed = 0;
				current_page_table->cache_disable = 0;
				current_page_table->write_through = 1;
				current_page_table->user = 0;
				current_page_table->read_write = 1;
				current_page_table->present = 1;
				current_page += PAGE_SIZE;
			}
		}

		current_pages = 1024;
		base = (void *)0;
	}
	sti();
}

void free_page_table(PAGE_TABLE page_table_index){ //free page tables resources
	cli();
	page_table_descriptor *current_page_table_descriptor = (page_table_descriptor *)get_list_element(page_table_list, page_table_index);
	int i, j;
	page_table_entry empty_page_table;
	empty_page_table.ignored = 1;
	empty_page_table.present = 0;
	for(i = 0; i < 1024; ++i){ //free the page tables
		page_directory_entry *current_page_directory = current_page_table_descriptor->page_table + i;
		if(!current_page_directory->ignored){
			for(j = 0; j < 1024; ++j){
				page_table_entry *current_page_table = (page_table_entry *)(current_page_directory->page_table<<12) + j;
				memcpy(current_page_table, &empty_page_table, sizeof(page_table_entry));
			}	
		}	
	}

	list_node *current_memory_page_block_node = memory_page_map->first;
	memory_page_block *prev_memory_page_block = 0;
	while(current_memory_page_block_node){ //free the allocated memory
		memory_page_block *current_memory_page_block = (memory_page_block *)current_memory_page_block_node->value;
		
		if(current_memory_page_block->page_table == page_table_index && (int)current_memory_page_block->base > 0x100000){
			if(prev_memory_page_block && prev_memory_page_block->page_table == KERNEL_PAGE_TABLE){
				prev_memory_page_block->limit += current_memory_page_block->limit;	
				remove_from_list(memory_page_map, current_memory_page_block);
				current_memory_page_block_node = current_memory_page_block_node->next;
			}else if(current_memory_page_block_node->next){
				memory_page_block *next_memory_page_block = (memory_page_block *)current_memory_page_block_node->next->value;
				if(next_memory_page_block->page_table == KERNEL_PAGE_TABLE){
					next_memory_page_block->limit += current_memory_page_block->limit;
					next_memory_page_block->base = current_memory_page_block->base;
					remove_from_list(memory_page_map, current_memory_page_block);
					current_memory_page_block_node = current_memory_page_block_node->next->next;
					prev_memory_page_block = (memory_page_block *)current_memory_page_block_node->next->value;
				}else{
					current_memory_page_block->bound = 0;
					current_memory_page_block->page_table = KERNEL_PAGE_TABLE;
					prev_memory_page_block = (memory_page_block *)current_memory_page_block_node->value;
					current_memory_page_block_node = current_memory_page_block_node->next;
				}
			}else{
				current_memory_page_block->bound = 0;
				current_memory_page_block->page_table = KERNEL_PAGE_TABLE;
				prev_memory_page_block = (memory_page_block *)current_memory_page_block_node->value;
				current_memory_page_block_node = current_memory_page_block_node->next;
			}
		}else{	
			prev_memory_page_block = (memory_page_block *)current_memory_page_block_node->value;
			current_memory_page_block_node = current_memory_page_block_node->next;
		}
	}

	current_page_table_descriptor->bound = 0;
	sti();
}
