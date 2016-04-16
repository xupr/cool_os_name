#include "../headers/process.h"
#include "../headers/memory.h"
#include "../headers/list.h"
#include "../headers/heap.h"
#include "../headers/screen.h"
#include "../headers/string.h"
#include "../headers/filesystem.h"
#include "../headers/interrupts.h"
#include "../headers/portio.h"

#define PROCESS_CODE_BASE 0x600000
#define KERNEL_BASE 0x100000
#define KERNEL_LIMIT 0x4FFFFF

#define PIT_COMMAND_REGISTER 0x43
#define PIT_CHANNEL0_DATA_PORT 0x40

typedef enum{
	CREATED,
	WAITING,
	RUNNING,
	BLOCKED
} PROCESS_STATE;

typedef struct{
//	unsigned int eflags;
	unsigned int edi;
	unsigned int esi;
	unsigned int ebp;
	unsigned int ignored_esp;
	unsigned int ebx;
	unsigned int edx;
	unsigned int ecx;
	unsigned int eax;
	unsigned int eip;
	unsigned int cs;
	unsigned int eflags;
	unsigned int esp;
	unsigned int ss;
} registers;

typedef struct{
	PAGE_TABLE page_table;
	void *heap_start;
	list *open_files;
	PROCESS_STATE state;
	int quantum;
	int screen_index;
	int kernel_stack;
	registers *regs;
} process_descriptor;

typedef struct{
	int used;
	FILE fd;
	int file_offset;
} open_file;

typedef struct{
	char present : 1;
	char wrtie : 1;
	char user : 1;
	char reserved_write : 1;
	char instruction_fetch : 1;
} page_fault_error_code;

void *pit_interrupt_entry;
static list *process_list;
static char tss[104];
static int free_kernel_stack = 0x4FFFFF;
static int current_process = 0;
//void (*jump_to_ring3)(int offset);
extern void jump_to_ring3(int offset);
static int screen_index_for_shell = 0;

void switch_kernel_stack(int new_kernel_stack){
	*(int *)(tss + 4) = new_kernel_stack;
}

void init_process(void){
	char gdt[6];
	asm("sgdt %0" : "=m"(gdt));
	char *c = *((char **)(gdt + 2)) + 5*8;
	*(short *)(c+2) = (short)((int)tss & 0xFFFF);
	*(c+4) = (char)(((int)tss>>16) & 0xFF);
	*(c+7) = (char)(((int)tss>>24) & 0xFF);
	//print(itoa((int)tss));
	*(int *)(tss + 4) = 0x4FFFFF;
	*(short *)(tss + 8) = 0x10;
	*(short *)(tss + 102) = 104;
	asm("mov ax, 0x2B; ltr ax");
	print("--tss initialized\n");
	
	outb(PIT_COMMAND_REGISTER, 0b00110100);
	outb(PIT_CHANNEL0_DATA_PORT, 0);
	outb(PIT_CHANNEL0_DATA_PORT, 0);
	create_IDT_descriptor(0x20, (int)&pit_interrupt_entry, 0x8, 0x8E);

	process_list = create_list();
/*	process_descriptor *kernel_process = (process_descriptor *)malloc(sizeof(process_descriptor));
	kernel_process->page_table = 0;
	kernel_process->state = RUNNING;
	kernel_process->open_files = 0;
	add_to_list(process_list, kernel_process);
*/}

int find_process_to_run(void){
	list_node *process_node = process_list->first;
	int i = 0;
	int found = 0;
	while(process_node){
		process_descriptor *process = (process_descriptor *)process_node->value;
		if(found)
			if(process->state == CREATED || process->state == WAITING)
				return i;
		
		if(process->state == RUNNING)
			found = 1;

		process_node = process_node->next;
		++i;
	}

	return 0;
}

void pit_interrupt_handler(registers *regs){
	//print("kappa123");
	//registers *regs = regs_end+1;
	switch_memory_map(KERNEL_PAGE_TABLE);
	list_node *process_node = process_list->first;
	process_descriptor *process = (process_descriptor *)process_node->value;
	if(process_list->length == 0){
		execute("shell.o", screen_index_for_shell++);
		send_EOI(0);
		return;			
	}
	
	if(process_list->length == 1){
		if(process->state == CREATED){
			print("running created process\n");
			process->state = RUNNING;
			process->quantum = 5;
			process->regs = (registers *)malloc(sizeof(registers));
			regs->esp = 0x9FFFFF;
			regs->cs = 0x1B;
			regs->ss = 0x23;
			regs->eip = PROCESS_CODE_BASE;
			//print(itoa(regs->eflags));
			switch_kernel_stack(process->kernel_stack);
			switch_memory_map(process->page_table);
			send_EOI(0);
			return;
		}
		
		//print(itoa(regs->eip));
		if(process->quantum > 1)
			--process->quantum;

		execute("shell.o", screen_index_for_shell++);
		switch_memory_map(process->page_table);
		send_EOI(0);
		return;
	}

	if(process_list->length <= 3){
		execute("shell.o", screen_index_for_shell++);
	}

	while(process_node){
		process = (process_descriptor *)process_node->value;
		if(process->state == RUNNING){
			/*print("running process quantum: ");
			print(itoa(process->quantum));
			print("\n");*/
			if(process->quantum-- < 1){
				int process_to_run_index = find_process_to_run();
				if(process_to_run_index == -1){
					print("asd");
					process->quantum = 5;
					switch_memory_map(process->page_table);
					return;
				}

				process_descriptor *process_to_run = get_list_element(process_list, process_to_run_index);

				//print("switching processes to ");
				//print(itoa(process_to_run_index));
				//print("\n");
				if(process_to_run->state == CREATED){
					memcpy(process->regs, regs, sizeof(registers));
					process->state = WAITING;

					print("running created process\n");
					current_process = process_to_run_index;
					process_to_run->state = RUNNING;
					process_to_run->quantum = 5;
					process_to_run->regs = (registers *)malloc(sizeof(registers));
					regs->esp = 0x9FFFFF;
					regs->cs = 0x1B;
					regs->ss = 0x23;
					regs->eip = PROCESS_CODE_BASE;
					//print(itoa(regs->eflags));
					switch_kernel_stack(process_to_run->kernel_stack);
					switch_memory_map(process_to_run->page_table);
					send_EOI(0);
					return;
				}

				if(process_to_run->state == WAITING){
					memcpy(process->regs, regs, sizeof(registers));
					process->state = WAITING;
					
					current_process = process_to_run_index;
					process_to_run->state = RUNNING;
					process_to_run->quantum = 5;
					memcpy(regs, process_to_run->regs, sizeof(registers));
					switch_kernel_stack(process_to_run->kernel_stack);
					switch_memory_map(process_to_run->page_table);
					send_EOI(0);
					return;
				}
				print("process quantum ended");
			}
			break;
		}
		
		process_node = process_node->next;
	}
	switch_memory_map(process->page_table);
	//print("how the fuck did i get here?");
	send_EOI(0);
	return;
}

FILE fopen(char *file_name){
	FILE fd = open(file_name);
	process_descriptor *process = (process_descriptor *)get_list_element(process_list, current_process);
	
	list_node *current_open_file_node = process->open_files->first;
	FILE vfd = 0;
	while(current_open_file_node){
		open_file *current_open_file = (open_file *)current_open_file_node->value;
		if(!current_open_file->used)
			break;

		++vfd;
		current_open_file_node = current_open_file_node->next;
	}

	open_file *file;
	if(!current_open_file_node){
		file = (open_file *)malloc(sizeof(open_file));
		add_to_list(process->open_files, file);
	}else
		file = (open_file *)current_open_file_node->value;

	file->fd = fd;
	file->file_offset = 0;
	add_to_list(process->open_files, file);
	return vfd;
}

int fread(char *buff, int count, FILE fd){
	process_descriptor *process = (process_descriptor *)get_list_element(process_list, current_process);
	open_file *file = get_list_element(process->open_files, fd);
	seek(file->fd, file->file_offset);
	int bytes_read = read(file->fd, buff, count);
	file->file_offset += bytes_read;
	return bytes_read;
}

int fwrite(char *buff, int count, FILE fd){
	process_descriptor *process = (process_descriptor *)get_list_element(process_list, current_process);
	open_file *file = get_list_element(process->open_files, fd);
	seek(file->fd, file->file_offset);
	int bytes_written = write(file->fd, buff, count);
	file->file_offset += bytes_written;
	return bytes_written;
}

void fclose(FILE fd){
	process_descriptor *process = (process_descriptor *)get_list_element(process_list, current_process);
	open_file *file = get_list_element(process->open_files, fd);
	
	int found = 0;
	list_node *current_process_descriptor_node = process_list->first;
	while(current_process_descriptor_node && !found){
		process_descriptor *current_process_descriptor = (process_descriptor *)current_process_descriptor_node->value;	
		list_node *current_open_file_node = current_process_descriptor->open_files->first;
		while(current_open_file_node && !found){
			open_file *current_open_file = (open_file *)current_open_file_node->value;
			if(current_open_file->used && current_open_file->fd == file->fd)
				found = 1;

			current_open_file_node = current_open_file_node->next;
		}

		current_process_descriptor_node = current_process_descriptor_node->next;
	}

	if(!found)
		close(file->fd);

	file->used = 0;
}

void handle_page_fault(void *address, int fault_info){
	set_vga_colors(WHITE, RED);
	print("page fault: ");
	if(!(*(page_fault_error_code *)&fault_info).present){
		set_vga_colors(WHITE, RED);
		print("page not present\n");
		process_descriptor *current_process_descriptor = (process_descriptor *)get_list_element(process_list, current_process);
		allocate_memory(current_process_descriptor->page_table, address, 1);
	}else{
		set_vga_colors(WHITE, RED);
		print("no premissions to enter page\n");
stop:
		asm("hlt");
		goto stop;
	}
}

void *get_heap_start(PID process_index){
	return ((process_descriptor *)get_list_element(process_list, process_index))->heap_start;
}

void create_process(char *code, int length, int screen_index){
//	print(itoa(length));
	process_descriptor *process = (process_descriptor *)malloc(sizeof(process_descriptor));
	process->page_table = create_page_table();
	//print(itoa(process->page_table));
	process->heap_start = (void *)(PROCESS_CODE_BASE + length);
	process->open_files = create_list();
	process->state = CREATED;
	process->screen_index = screen_index;
	process->kernel_stack = free_kernel_stack;
	free_kernel_stack -= 0x10000;
	add_to_list(process_list, process);
	identity_page(process->page_table, (void *)KERNEL_BASE, KERNEL_LIMIT);
//	identity_page(process->page_table, (void *)SCREEN, SCREEN_END - SCREEN - 1);
	identity_page(process->page_table, (void *)0x0, 0xfffff);
	allocate_memory(process->page_table, (void *)PROCESS_CODE_BASE, length);
	write_virtual_memory(process->page_table, code, (void *)PROCESS_CODE_BASE, length);
//	print("allah");
	//switch_memory_map(process->page_table);
	//print(" akbar\n");
	//jump_to_ring3(PROCESS_CODE_BASE);
	//asm("JMP 0x18:0x600000"); //: : "a"(PROCESS_CODE_BASE));
	//switch_memory_map(KERNEL_PID);
}

PID get_current_process(void){
	return (PID)current_process;
}

int get_process_screen_index(PID process_index){
	return ((process_descriptor *)get_list_element(process_list, process_index))->screen_index;
}
