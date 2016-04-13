#include "../headers/memory.h"
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
	PID process;
} memory_page_block;

typedef struct {
	int bound;
	page_directory_entry *page_table;
} page_table_descriptor;

static page_directory_entry kernel_page_directory[1024]__attribute__((aligned(PAGE_SIZE)));
static page_table_entry kernel_page_table[8*1024]__attribute__((aligned(PAGE_SIZE)));
static list *page_table_list;
/*static memory_page_block kernel_pages = {1, 1, (void *)0x100000, 0x500000, KERNEL_PID};
static memory_page_block first_page_block = {0, 0, (void *)0, 0, 0};
static list_node first_memory_page_map = {&first_page_block, 0};
static list_node kernel_memory_page_map = {&kernel_pages, &first_memory_page_map};
static list _memory_page_map = {&kernel_memory_page_map, 2};*/
static list *memory_page_map;

static PAGE_TABLE kernel_page_table_index = 0;

static int compare_memory_blocks(void *a, void *b){
	unsigned int c = (unsigned int)(((memory_information_block *)a)->base), d = (unsigned int)(((memory_information_block *)b)->base);
/*	print(itoa(c));
	print(":");
	print(itoa(d));
	print("  ");
*/	if(c > d)
		return 1;
	return -2;
}

void init_memory(char *memory_map_length, void *memory_map){
	memory_page_map = create_list();
	int i, j;
/*	for(i = 0; i < *memory_map_length; ++i){ //won't work because the list is unsorted, could sort first.
		memory_information_block *current_block = (memory_information_block *)memory_map+i;
		print(itoa((int)current_block->base));
		print("   ");
		print(itoa(current_block->limit));
		print("   ");
		print(itoa(current_block->type));
		print("\n");
	}
*/	sort(memory_map, sizeof(memory_information_block), *memory_map_length, compare_memory_blocks);
	for(i = 0; i < *memory_map_length; ++i){ //won't work because the list is unsorted, could sort first.
		memory_information_block *current_block = (memory_information_block *)memory_map+i;
		memory_page_block *page_block = (memory_page_block *)malloc(sizeof(memory_page_block));
		if(current_block->type == USABLE && (int)current_block->base >= 0x100000){
			if((int)current_block->base >= 0x100000 && (int)current_block->base < 0x600000){
				/*first_page_block.bound = 0;
				first_page_block.pinned = 0;
				first_page_block.base = (void *)0x600000;
				first_page_block.limit = (int)current_block->base + current_block->limit - 0x600000;
				first_page_block.process = KERNEL_PIs(itoa(((memory_information_block *)a)->base))ge_block;*/
				memory_page_block *kernel_page_block = (memory_page_block *)malloc(sizeof(memory_page_block));
				kernel_page_block->bound = 1;
				kernel_page_block->pinned = 1;
				kernel_page_block->base = (void *)0x100000;
				kernel_page_block->limit = 0x4FFFFF;
				kernel_page_block->process = -1;

				page_block->bound = 0;
				page_block->pinned = 0;
				page_block->base = (void *)0x600000;
				page_block->limit = (int)current_block->base + current_block->limit - 0x600000 - 1;
				page_block->process = -1;
			}else{
				page_block->bound = 0;
				page_block->pinned = 0;
				page_block->base = current_block->base;
				page_block->limit = current_block->limit;
				page_block->process = -1;
			}

		}else{
			page_block->bound = 1;
			page_block->pinned = 1;
			page_block->base = current_block->base;
			page_block->limit = current_block->limit;
			page_block->process = -1;
		}

		add_to_list(memory_page_map, page_block);
		/*print(itoa((int)current_block->base));
		print("   ");
		print(itoa(current_block->limit));
		print("   ");
		print(itoa(current_block->type));
		print("\n");*/
	}
	
	print("--memory map inititalized\n");

	//print(itoa((int)kernel_page_table));
	//print("\n");
	//print(itoa(sizeof(page_table_entry)));
	//page_directory_entry d = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1};
	//print(itoa(*((int *)&d)));
	//print(itoa(((int)kernel_page_table)>>12));
/*	for(i = 0; i < 8; ++i){
		page_directory_entry current_page_directory;
		current_page_directory.page_table = ((int)kernel_page_table+4*1024*i)>>12;
		current_page_directory.available = 0;
		current_page_directory.ignored = 0;
		current_page_directory.size = 0;
		current_page_directory.zero = 0;
		current_page_directory.accessed = 0;
		current_page_directory.cache_disable = 0;
		current_page_directory.write_through = 1;
		current_page_directory.user = 0;
		current_page_directory.read_write = 1;
		current_page_directory.present = 1;
		kernel_page_directory[i] = current_page_directory;
		for(j = 0; j < 1024; ++j){
			page_table_entry current_page_table;
			current_page_table.physical_page = (int)(4*1024*(i*1024+j))>>12;
			current_page_table.available = 0;
			current_page_table.ignored = 0;
			current_page_table.zero = 0;
			current_page_table.dirty = 0;
			current_page_table.accessed = 0;
			current_page_table.cache_disable = 0;
			current_page_table.write_through = 1;
			current_page_table.user = 0;
			current_page_table.read_write = 1;
			current_page_table.present = 1;
			kernel_page_table[i*1024 + j] = current_page_table;
		}
	}

	page_directory_entry current_page_directory;
	current_page_directory.ignored = 1;
	for(; i < 1024; kernel_page_directory[i++] = current_page_directory);
*/		
	//asm("mov cr3, eax; mov eax, cr0; or eax, 0x80000000; mov cr0, eax" : : "a"(kernel_page_directory));
	page_table_list = create_list();
//	page_table_descriptor *os_page_table = (page_table_descriptor *)malloc(sizeof(page_table_descriptor));
//	os_page_table->bound = 1;
	PAGE_TABLE os_page_table_index = create_page_table();
	identity_page(os_page_table_index, (void *)0, 32*1024*1024-1);
//	add_to_list(page_table_list, os_page_table);
//	asm("int 3");
	switch_memory_map(kernel_page_table_index);
	asm("mov eax, cr0; or eax, 0x80000000; mov cr0, eax");
	print("--paging initiailzed\n");
	//create_page_table();
}

PAGE_TABLE create_page_table(){
	list_node *current = page_table_list->first;
	char found = 0;
	PAGE_TABLE page_table_index = 0;
	page_table_descriptor *current_page_table_descriptor;
	while(current != 0){
		if(!((page_table_descriptor *)current->value)->bound){
			found = 1;
			((page_table_descriptor *)current->value)->bound = 1;
			current_page_table_descriptor = (page_table_descriptor *)current->value;
			break;
		}
		
		++page_table_index;
		current = current->next;
	}
	
	if(!found){
		current_page_table_descriptor = (page_table_descriptor *)malloc(sizeof(page_table_descriptor));
		current_page_table_descriptor->page_table = (page_directory_entry *)malloc(2048*sizeof(page_directory_entry));
		//print(itoa((int)(current_page_table_descriptor->page_table)));
		//print("\n");
		page_directory_entry *page_table_address = (page_directory_entry *)((int)current_page_table_descriptor->page_table & (int)(~0xFFF));
		if(page_table_address < current_page_table_descriptor->page_table)
			page_table_address = (page_directory_entry *)((char *)page_table_address + PAGE_SIZE);
		current_page_table_descriptor->page_table = page_table_address;
		//print(itoa((int)(current_page_table_descriptor->page_table)));
		//print("\n");
		current_page_table_descriptor->bound = 1;
		add_to_list(page_table_list, current_page_table_descriptor);
	}

	//print(itoa(page_table_index));

	int i;
	page_directory_entry current_page_directory;
	current_page_directory.ignored = 1;
	/*print(itoa((int)kernel_page_directory));
	print("\n");
	print(itoa((int)current_page_table_descriptor->page_table));
	print("\n");*/
	for(i = 0; i < 1024; current_page_table_descriptor->page_table[i++] = current_page_directory);
//	print("\n");
//	print(itoa((int)current_page_directory.ignored));
//	print(itoa((int)(current_page_table_descriptor->page_table + 1)));
//	print("kappa\n");
	return page_table_index; 
	//for
	//print(itoa((unsigned int)current_page_table_descriptor));
}

void switch_memory_map(PAGE_TABLE page_table_index){
	page_table_descriptor *current_page_table_descriptor = get_list_element(page_table_list, (int)page_table_index);
	asm("mov cr3, eax" : : "a"(current_page_table_descriptor->page_table));
}

void allocate_memory(PAGE_TABLE page_table_index, void *base, int limit){
	int pages = (limit + ((int)base)%PAGE_SIZE - 1)/PAGE_SIZE + 1;
	int page_directories = (limit + ((int)base)%0x400000 - 1)/0x400000 + 1;
	int i;
	int current_pages = 1024 - (((int)base>>12) & 0x3FF);
	page_directory_entry *current_page_directory = (((page_table_descriptor *)get_list_element(page_table_list, page_table_index))->page_table + ((int)base>>22));
//	print(itoa((int)current_page_directory));
//	print("kappa\n");
	for(i = 0; i++ < page_directories; ++current_page_directory){
		page_table_entry *current_page_table;
		if(current_page_directory->ignored){
			//print("nope1\n");
			current_page_table = (page_table_entry *)malloc(2048*sizeof(page_table_entry));
			if((int)current_page_table & (int)(~0xFFF) > (int)current_page_table)
				current_page_table = (page_table_entry *)((int)current_page_table & (int)(~0xFFF));
			else
				current_page_table = (page_table_entry *)(((int)current_page_table & (int)(~0xFFF)) + PAGE_SIZE);
			
//			print(itoa((int)current_page_table));
//			print("\n");
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
//		print(itoa((int)current_page_table));
//		print("a\n");
		int j;
		for(j = 0; j++ < current_pages && pages--; ++current_page_table){
			print("321\n");
			if(current_page_table->ignored){
				//print("nope2\n");
				list_node *current_memory_page_block = memory_page_map->first;
				int index = 0;
				while(current_memory_page_block){
					if(!((memory_page_block *)current_memory_page_block->value)->bound && \
					(int)((memory_page_block *)current_memory_page_block->value)->base > 0x100000)
						break;

					++index;
					current_memory_page_block = current_memory_page_block->next;
				}

//				print(itoa(index));
//				print("\n");
				
				if(index < memory_page_map->length){
					void *page_address;
					if(index != memory_page_map->length - 1 &&\
					((memory_page_block *)(current_memory_page_block->next->value))->process == get_current_process()){
						//print("safta1\n");
						((memory_page_block *)current_memory_page_block->next->value)->base -= PAGE_SIZE;
						page_address = ((memory_page_block *)current_memory_page_block->next->value)->base;
						((memory_page_block *)current_memory_page_block->next->value)->limit += PAGE_SIZE;
						((memory_page_block *)current_memory_page_block->value)->limit -= PAGE_SIZE;
					}else if(index != 0 &&\
					((memory_page_block *)get_list_element(memory_page_map, index - 1))->process == get_current_process()){
					//	memory_page_block *temp_page_block = (memory_page_block *)get_list_element(index - 1);
						//print("safta2\n");
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
						temp_page_block->process = get_current_process();
						add_to_list_at(memory_page_map, temp_page_block, index);
//						print(itoa(((memory_page_block *)get_list_element(memory_page_map, index))->process));
//						print("\n");
						page_address = temp_page_block->base;
						((memory_page_block *)current_memory_page_block->value)->base += PAGE_SIZE;
						((memory_page_block *)current_memory_page_block->value)->limit -= PAGE_SIZE;
					}
				
					if(((memory_page_block *)current_memory_page_block->value)->limit < 0){
						remove_from_list(memory_page_map, current_memory_page_block);
						free(current_memory_page_block);
					}
					
					//print(itoa((int)page_address));
					//print("o\n");

					current_page_table->physical_page = (int)page_address>>12;
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
}

void write_virtual_memory(PAGE_TABLE page_table_index, char *source, void *base, int limit){	
	int pages = (limit + ((int)base)%PAGE_SIZE - 1)/PAGE_SIZE + 1;
	int page_directories = (limit + ((int)base)%0x400000 - 1)/0x400000 + 1;
	page_directory_entry *current_page_directory = ((page_table_descriptor *)get_list_element(page_table_list, page_table_index))->page_table + ((int)base>>22);
	//print(itoa((int)(((page_table_descriptor *)get_list_element(page_table_list, page_table_index))->page_table)));
	//print("\n");
	int i;
	int current_pages = 1024 - (((int)base>>12) & 0x3FF);
	for(i = 0; i++ < page_directories; ++current_page_directory){
		//if(current_page_directory->ignored)
			//print("wtf123\n");
		page_table_entry *current_page_table;
		current_page_table = ((page_table_entry *)(current_page_directory->page_table<<12)) + (((int)base>>12) & 0x3FF);
		int j;
		for(j = 0; j++ < current_pages && pages--; ++current_page_table){
			//print(itoa(((current_page_table->physical_page)<<12)));
			memcpy((char *)((current_page_table->physical_page)<<12), source, (limit - 1)%PAGE_SIZE + 1);
			limit -= PAGE_SIZE;
			source += PAGE_SIZE;
		}
		
		base = (void *)0;
		current_pages = 1024;
	}
}

void identity_page(PAGE_TABLE page_table_index, void *base, int limit){
/*	void *base = (void *)KERNEL_BASE;
	int limit = KERNEL_LIMIT;
*/	int pages = (limit + ((int)base)%PAGE_SIZE - 1)/PAGE_SIZE + 1;
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
//		print(itoa((current_page_directory->page_table<<12)));
//		print("a\n");
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
}
