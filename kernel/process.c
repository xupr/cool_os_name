//REMINDER: kernel stacks may overflow to the heap.
#include "../headers/process.h"
#include "../headers/kernel.h"
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

typedef struct {
	unsigned char bound;
	unsigned short access;
	unsigned int creator_uid;
	unsigned int size;
	unsigned int address_block;
	unsigned short name_address;
	unsigned int creation_date;
	unsigned int update_date;
} inode;

typedef enum{
	CREATED,
	WAITING,
	RUNNING,
	BLOCKED,
	EXITED
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

typedef struct _process_descriptor{
	char *image_name;
	PAGE_TABLE page_table;
	void *heap_start;
	list *open_files;
	PROCESS_STATE state;
	int quantum;
	int screen_index;
	int kernel_stack;
	registers *regs;
	struct _process_descriptor *parent;
	int ruid;
	int euid;
} process_descriptor;

typedef struct{
	int used;
	FILE fd;
	int file_offset;
	char read;
	char write;
} open_file;

typedef struct{
	char present : 1;
	char write : 1;
	char user : 1;
	char reserved_write : 1;
	char instruction_fetch : 1;
} page_fault_error_code;

void *pit_interrupt_entry;
static list *process_list;
static char tss[104];
static int free_kernel_stack = 0x4FFFFF;
static int current_process = -1;
//void (*jump_to_ring3)(int offset);
extern void jump_to_ring3(int offset);
static int screen_index_for_shell = 0;
static int exited_process = 0;

void dump_process_list(void){
	cli();
	list_node *current_process_descriptor_node = process_list->first;
	int pid = 0;
	print("\n");
	print("PID\timage name");
	set_tab_size(12);
	print("\t");
	set_tab_size(8);
	print("state\tquantum\tscreen\tkernel stack\tpage table\n");
	while(current_process_descriptor_node){
		process_descriptor *current_process_descriptor = (process_descriptor *)current_process_descriptor_node->value;
		char *image_name = (char *)malloc(strlen(current_process_descriptor->image_name) + 1);
		strcpy(image_name, current_process_descriptor->image_name);
		print(itoa(pid));
		print("\t");
		print(strtok(image_name, "."));
		set_tab_size(12);
		print("\t");
		set_tab_size(8);
		switch(current_process_descriptor->state){
			case CREATED:
				print("CREATED");
				break;
			case RUNNING:
				print("RUNNING");
				break;
			case WAITING:
				print("WAITING");
				break;
			case BLOCKED:
				print("BLOCKED");
				break;
		}
		print("\t");
		print(itoa(current_process_descriptor->quantum));
		print("\t");
		print(itoa(current_process_descriptor->screen_index));
		print("\t");
		print(itoa(current_process_descriptor->kernel_stack));
		print("\t\t");
		print(itoa(current_process_descriptor->page_table));
		print("\n");
		free(image_name);

		current_process_descriptor_node = current_process_descriptor_node->next;
		++pid;
	}
	print("\n");
	sti();
}

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
	int process_to_run = current_process+1;
	if(process_to_run >= process_list->length)
		process_to_run = 0;

	list_node *process_node = process_list->first;
	int i = 0;
	while(process_node){
		process_descriptor *process = (process_descriptor *)process_node->value;
		if(i >= process_to_run)
			if(process->state == CREATED || process->state == WAITING)
				return i;

		process_node = process_node->next;
		++i;
	}
	
	i = 0;
	process_node = process_list->first;
	while(process_node){
		process_descriptor *process = (process_descriptor *)process_node->value;
		if(process->state == CREATED || process->state == WAITING)
			return i;

		process_node = process_node->next;
		++i;
	}

	return current_process;
	/*list_node *process_node = process_list->first;
	int i = 0;
	int found = 0;
	while(process_node){
		process_descriptor *process = (process_descriptor *)process_node->value;
		if(found)
			if(process->state == CREATED || process->state == WAITING)
				return i;
		
		if(process->state == RUNNING || (process->state == BLOCKED && process->quantum != 0))
			found = 1;

		process_node = process_node->next;
		++i;
	}

	return 0;*/
}

void copy_registers(registers *dst, registers *src){
	if(src->cs & 3){
		memcpy(dst, src, sizeof(registers));
		return;
	}

	memcpy(dst, src, sizeof(registers) - 2*4);
}

void switch_process(PID new_process_index, registers *regs){
	process_descriptor *new_process = (process_descriptor *)get_list_element(process_list, new_process_index);
	if(new_process->state == CREATED){
		/*print("switching from PID ");
		if(current_process != -1)
			print(itoa(current_process));
		else
			print("-1");*/
		print("running created process with PID ");
		print(itoa(new_process_index));
		print("\n");
		current_process = new_process_index;
		new_process->state = RUNNING;
		new_process->quantum = 5;
		new_process->regs = (registers *)malloc(sizeof(registers));
		regs->esp = 0x9FFFFF;
		regs->cs = 0x1B;
		regs->ss = 0x23;
		regs->eip = PROCESS_CODE_BASE;
		//print(itoa(regs->eflags));
		switch_kernel_stack(new_process->kernel_stack);
		/*print(itoa(new_process->page_table));*/
		switch_memory_map(new_process->page_table);
		return;
	}

	/*print("switching from PID ");
	if(current_process != -1)
		print(itoa(current_process));
	else
		print("-1");
	print(" to PID ");
	print(itoa(new_process_index));
	print("\n");*/
	if(new_process->state == WAITING){
		current_process = new_process_index;
		new_process->state = RUNNING;
		new_process->quantum = 5;
		memcpy(regs, new_process->regs, sizeof(registers));
		if((regs->cs & 3) == 0)
			regs->esp = new_process->kernel_stack; 
			
		switch_kernel_stack(new_process->kernel_stack);
		switch_memory_map(new_process->page_table);
		return;
	}
}

void pit_interrupt_handler(registers *regs){
	switch_memory_map(KERNEL_PAGE_TABLE);
	if(process_list->length == 0){
		send_EOI(0);
		return;
	}

	if(current_process == -1){
		switch_process(find_process_to_run(), regs);
		send_EOI(0);
		return;
	}

	process_descriptor *process = (process_descriptor *)get_list_element(process_list, current_process);
	PID process_to_run = find_process_to_run();
	if(process_to_run != current_process && (--process->quantum < 1 || process->state == BLOCKED)){
		memcpy(process->regs, regs, sizeof(registers));
		process->kernel_stack = (int)(regs+1);
		process->quantum = 0;
		if(process->state == RUNNING)
			process->state = WAITING;

		switch_process(find_process_to_run(), regs);
		send_EOI(0);
		return;
	}

	switch_memory_map(process->page_table);
	send_EOI(0);
	return;
}

FILE fopen(char *file_name, char *mode){
	process_descriptor *process = (process_descriptor *)get_list_element(process_list, current_process);
	inode *current_inode = (inode *)malloc(sizeof(inode)); 
	get_inode(file_name, current_inode);
	if(process->euid && current_inode->bound != 255){
		short relevant_access_bits;
		if(process->euid == current_inode->creator_uid)
			relevant_access_bits = (current_inode->access>>6);
		else
			relevant_access_bits = (current_inode->access&7);

		if(!strcmp(mode, "r") && !(relevant_access_bits&4))
			return -1;
		if(!strcmp(mode, "w") && !(relevant_access_bits&2))
			return -1;
		if(!strcmp(mode, "r+") && (!(relevant_access_bits&2) || !(relevant_access_bits&4)))
			return -1;
	}
	
	FILE fd = open(file_name);
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
	if(!strcmp(mode, "r")){
		file->read = 1;
		file->write = 0;
	}else if(!strcmp(mode, "w")){
		file->read = 0;
		file->write = 1;
	}else if(!strcmp(mode, "r+")){
		file->read = 1;
		file->write = 1;
	}
	add_to_list(process->open_files, file);
	return vfd;
}

int fread(char *buff, int count, FILE fd){
	if(fd == -1)
		return -1;
	process_descriptor *process = (process_descriptor *)get_list_element(process_list, current_process);
	open_file *file = get_list_element(process->open_files, fd);
	if(!file->read)
		return -1;
	seek(file->fd, file->file_offset);
	int bytes_read = read(file->fd, buff, count);
	file->file_offset += bytes_read;
	return bytes_read;
}

int fwrite(char *buff, int count, FILE fd){
	if(fd == -1)
		return -1;
	process_descriptor *process = (process_descriptor *)get_list_element(process_list, current_process);
	open_file *file = get_list_element(process->open_files, fd);
	if(!file->write)
		return -1;
	seek(file->fd, file->file_offset);
	int bytes_written = write(file->fd, buff, count);
	file->file_offset += bytes_written;
	return bytes_written;
}

void fclose(FILE fd){
	if(fd == -1)
		return;
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
	print("page fault on address ");
	set_vga_colors(WHITE, RED);
	print(itoa((int)address));
	set_vga_colors(WHITE, RED);
	print(": ");
	set_vga_colors(WHITE, RED);
	if((*(page_fault_error_code *)&fault_info).write)
		print("WRITE, ");
	else
		print("READ, ");
	set_vga_colors(WHITE, RED);
	if((*(page_fault_error_code *)&fault_info).user)
		print("USER, ");
	else
		print("KERNEL, ");
	set_vga_colors(WHITE, RED);
	if((*(page_fault_error_code *)&fault_info).reserved_write)
		print("RESERVED WRITE, ");
	set_vga_colors(WHITE, RED);
	if((*(page_fault_error_code *)&fault_info).instruction_fetch)
		print("INSTRUCTION FETCH, ");
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

void create_process(char *code, int length, int screen_index, char *file_name){
	cli();
	/*print(itoa(*code));*/
	process_descriptor *process = (process_descriptor *)malloc(sizeof(process_descriptor));
	process->image_name = (char *)malloc(strlen(file_name) + 1);
	strcpy(process->image_name, file_name);
	process->page_table = create_page_table();
	//print(itoa(process->page_table));
	process->heap_start = (void *)(PROCESS_CODE_BASE + length);
	process->open_files = create_list();
	process->state = CREATED;
	process->screen_index = screen_index;
	process->kernel_stack = free_kernel_stack;
	process->parent = 0;
	process->ruid = 0;
	process->euid = 0;
	free_kernel_stack -= 0x10000;
	add_to_list(process_list, process);
	/*PID _current_process = current_process;
	current_process = process_list->length - 1;*/
	identity_page(process->page_table, (void *)KERNEL_BASE, KERNEL_LIMIT);
//	identity_page(process->page_table, (void *)SCREEN, SCREEN_END - SCREEN - 1);
	identity_page(process->page_table, (void *)0x0, 0xfffff);
	allocate_memory(process->page_table, (void *)PROCESS_CODE_BASE, length);
	write_virtual_memory(process->page_table, code, (void *)PROCESS_CODE_BASE, length);
	sti();
}

void exit_process(int result_code){
	cli();
	print("process with PID ");
	print(itoa(current_process));
	print(" exited with result code ");
	print(itoa(result_code));
	print("\n");
	process_descriptor *process = (process_descriptor *)get_list_element(process_list, current_process);
	switch_memory_map(KERNEL_PAGE_TABLE);
	free_page_table(process->page_table);
	if(process->parent && process->parent->state == BLOCKED){
		/*print("resuming parent process\n");*/
		process->parent->state = WAITING;
	}
	//TODO clean other stuff up too lazy right now :(
	remove_from_list(process_list, process);
	process->state = EXITED;
	current_process = -1;
	sti_forced();

	while(1) asm("hlt");
}

PID get_current_process(void){
	return (PID)current_process;
}

int get_process_screen_index(PID process_index){
	return ((process_descriptor *)get_list_element(process_list, process_index))->screen_index;
}

void execute_from_process(char *file_name){
	cli();
	char *_file_name = (char *)malloc(strlen(file_name) + 1);
	strcpy(_file_name, file_name);
	file_name = _file_name;
	switch_memory_map(KERNEL_PAGE_TABLE);
	process_descriptor *process = (process_descriptor *)get_list_element(process_list, current_process);
	inode *current_inode = (inode *)malloc(sizeof(inode)); 
	get_inode(file_name, current_inode);
	if(process->euid && current_inode->bound != 255){
		short relevant_access_bits;
		if(process->euid == current_inode->creator_uid)
			relevant_access_bits = (current_inode->access>>6);
		else
			relevant_access_bits = (current_inode->access&7);
		
		if(!relevant_access_bits&1){
			sti();
			return;
		}
	}
	process->state = BLOCKED;
	execute(file_name, process->screen_index);
	process_descriptor *created_process = (process_descriptor *)get_list_element(process_list, process_list->length - 1);
	created_process->parent = process;
	created_process->ruid = process->euid;
	created_process->euid = process->euid;
	sti_forced();
	while(process->state == BLOCKED) 
		asm("hlt");
}

int seteuid(int new_euid){
	process_descriptor *process = (process_descriptor *)get_list_element(process_list, current_process);
	if(process->ruid == 0){
		process->euid = new_euid;
		return 0;
	}

	return -1;
}

int get_current_euid(void){
	process_descriptor *process = (process_descriptor *)get_list_element(process_list, current_process);
	return process->euid;
}
