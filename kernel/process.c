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

typedef enum{
	CREATED,
	WAITING,
	RUNNING,
	BLOCKED,
	EXITED
} PROCESS_STATE;

typedef struct{
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
	list *open_dirs;
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
	int used;
	DIR dd;
	int index;
} open_dir;

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
extern void jump_to_ring3(int offset);
static int screen_index_for_shell = 0;
static int exited_process = 0;

void dump_process_list(void){ //print data on active processes
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
	char gdt[6]; //initialize tss
	asm("sgdt %0" : "=m"(gdt));
	char *c = *((char **)(gdt + 2)) + 5*8;
	*(short *)(c+2) = (short)((int)tss & 0xFFFF);
	*(c+4) = (char)(((int)tss>>16) & 0xFF);
	*(c+7) = (char)(((int)tss>>24) & 0xFF);
	*(int *)(tss + 4) = 0x4FFFFF;
	*(short *)(tss + 8) = 0x10;
	*(short *)(tss + 102) = 104;
	asm("mov ax, 0x2B; ltr ax");
	print("--tss initialized\n");
	
	outb(PIT_COMMAND_REGISTER, 0b00110100); //initialize PIT
	outb(PIT_CHANNEL0_DATA_PORT, 0);
	outb(PIT_CHANNEL0_DATA_PORT, 0);
	create_IDT_descriptor(0x20, (int)&pit_interrupt_entry, 0x8, 0x8E);
	print("--PIT initialized\n");

	process_list = create_list();
}

PID find_process_to_run(void){ //find a process to run
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
}

void switch_process(PID new_process_index, registers *regs){
	process_descriptor *new_process = (process_descriptor *)get_list_element(process_list, new_process_index);
	if(new_process->state == CREATED){
		print("running created process with PID ");
		print(itoa(new_process_index));
		print("\n");
		memcpy(regs, new_process->regs, sizeof(registers));
		current_process = new_process_index;
		new_process->state = RUNNING;
		new_process->quantum = 5;
		switch_kernel_stack(new_process->kernel_stack);
		switch_memory_map(new_process->page_table);
		return;
	}

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

void pit_interrupt_handler(registers *regs){ //scheduler
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
	/*inode *current_inode = (inode *)malloc(sizeof(inode)); 
	get_inode(file_name, current_inode);
	if(process->euid && current_inode->bound != 255){ //check permissions
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
	}*/
	
	FILE fd = open(file_name, mode); //open the file
	if(fd == -1)
		return -1;
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
	file->used = 1;
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

DIR opendir_from_process(char *file_name){
	process_descriptor *process = (process_descriptor *)get_list_element(process_list, current_process);
	DIR dd = opendir(file_name); //open the file
	if(dd == -1)
		return -1;
	list_node *current_open_dir_node = process->open_dirs->first;
	DIR vdd = 0;
	while(current_open_dir_node){
		open_dir *current_open_dir = (open_dir *)current_open_dir_node->value;
		if(!current_open_dir->used)
			break;

		++vdd;
		current_open_dir_node = current_open_dir_node->next;
	}

	open_dir *dir;
	if(!current_open_dir_node){
		dir = (open_dir *)malloc(sizeof(open_dir));
		add_to_list(process->open_dirs, dir);
	}else
		dir = (open_dir *)current_open_dir_node->value;

	dir->dd = dd;
	dir->index = 0;
	dir->used = 1;
	return vdd;
}

int fread(char *buff, int count, FILE fd){ //read if permissions allow
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

int readdir_from_process(char *buff, int count, DIR dd){
	if(dd == -1)
		return -1;
	process_descriptor *process = (process_descriptor *)get_list_element(process_list, current_process);
	open_dir *dir = get_list_element(process->open_dirs, dd);
	char *file_name = readdir(dir->dd, dir->index);
	if(!file_name)
		return -1;

	if(strlen(file_name) > count){
		memcpy(buff, file_name, count - 1);
		buff[count - 1] = '\0';
	}else
		strcpy(buff, file_name);
	++dir->index;

	return 0;
}

int fwrite(char *buff, int count, FILE fd){ //write if permissions allow
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

void fclose(FILE fd){ //close file if possible (no other process uses it)
	if(fd == -1)
		return;
	process_descriptor *process = (process_descriptor *)get_list_element(process_list, current_process);
	open_file *file = get_list_element(process->open_files, fd);
	
	list_node *current_process_descriptor_node = process_list->first;
	while(current_process_descriptor_node){
		process_descriptor *current_process_descriptor = (process_descriptor *)current_process_descriptor_node->value;	
		if(current_process_descriptor != process){
			list_node *current_open_file_node = current_process_descriptor->open_files->first;
			while(current_open_file_node){
				open_file *current_open_file = (open_file *)current_open_file_node->value;
				if(current_open_file->used && current_open_file->fd == file->fd){
					file->used = 0;
					return;
				}

				current_open_file_node = current_open_file_node->next;
			}
		}

		current_process_descriptor_node = current_process_descriptor_node->next;
	}

	close(file->fd);
	file->used = 0;
}

void closedir_from_process(DIR dd){ //close dir if possible (no other process uses it)
	if(dd == -1)
		return;
	process_descriptor *process = (process_descriptor *)get_list_element(process_list, current_process);
	open_dir *dir = get_list_element(process->open_dirs, dd);
	
	list_node *current_process_descriptor_node = process_list->first;
	while(current_process_descriptor_node){
		process_descriptor *current_process_descriptor = (process_descriptor *)current_process_descriptor_node->value;	
		if(current_process_descriptor != process){
			list_node *current_open_dir_node = current_process_descriptor->open_dirs->first;
			while(current_open_dir_node){
				open_dir *current_open_dir = (open_dir *)current_open_dir_node->value;
				if(current_open_dir->used && current_open_dir->dd == dir->dd){
					dir->used = 0;
					return;
				}

				current_open_dir_node = current_open_dir_node->next;
			}
		}

		current_process_descriptor_node = current_process_descriptor_node->next;
	}

	closedir(dir->dd);
	dir->used = 0;
}

FILE dup(FILE oldfd){
	if(oldfd == -1)
		return -1;

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

	memcpy(file, get_list_element(process->open_files, oldfd), sizeof(open_file));
	return vfd;
}

FILE dup2(FILE oldfd, FILE newfd){
	if(oldfd == -1 || newfd == -1)
		return -1;
	
	fclose(newfd);
	process_descriptor *process = (process_descriptor *)get_list_element(process_list, current_process);
	memcpy(get_list_element(process->open_files, newfd), get_list_element(process->open_files, oldfd), sizeof(open_file));
	return newfd;
}

void handle_page_fault(void *address, int fault_info){ //handle page faults
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
	if(!(*(page_fault_error_code *)&fault_info).present){ //if the page isnt present allocate
		set_vga_colors(WHITE, RED);
		print("page not present\n");
		process_descriptor *current_process_descriptor = (process_descriptor *)get_list_element(process_list, current_process);
		allocate_memory(current_process_descriptor->page_table, address, 1);
	}else{ //if there is a permissions violation, exit the process
		set_vga_colors(WHITE, RED);
		print("no premissions to enter page\n");
		if((*(page_fault_error_code *)&fault_info).user)
			exit_process(-1);
	}
}

void *get_heap_start(PID process_index){ //return heap bottom of process
	return ((process_descriptor *)get_list_element(process_list, process_index))->heap_start;
}

void create_process(char *code, int length, FILE stdin, FILE stdout, char *file_name, int argc, char **argv){
	cli();
	/*print(itoa(*code));*/
	process_descriptor *process = (process_descriptor *)malloc(sizeof(process_descriptor)); //create a process descriptor
	process->image_name = (char *)malloc(strlen(file_name) + 1);
	strcpy(process->image_name, file_name);
	process->page_table = create_page_table();
	process->regs = (registers *)malloc(sizeof(registers));
	process->regs->cs = 0x1B;
	process->regs->ss = 0x23;
	process->regs->eip = PROCESS_CODE_BASE;
	process->regs->eflags = 1<<9;
	process->heap_start = (void *)(PROCESS_CODE_BASE + length);
	process->open_files = create_list();
	process->open_dirs = create_list();
	process->state = CREATED;
	/*process->screen_index = screen_index;*/
	process->kernel_stack = free_kernel_stack;
	process->parent = 0;
	process->ruid = 0;
	process->euid = 0;
	free_kernel_stack -= 0x10000;
	add_to_list(process_list, process);
	/*int io_file_length = strlen("/dev/ttyX");
	char *stdin_file = (char *)malloc(io_file_length + 1);
	strcpy(stdin_file, "/dev/ttyX");
	stdin_file[io_file_length - 1] = screen_index + 0x31;
	int _current_process = current_process;
	current_process = process_list->length - 1;
	fopen(stdin_file, "r");
	fopen(stdout_file, "w");
	free(stdin_file);
	current_process = _current_process;*/

	open_file *stdin_file = (open_file *)malloc(sizeof(open_file));
	stdin_file->fd = stdin;
	stdin_file->used = 1;
	stdin_file->file_offset = 0;
	stdin_file->write = 0;
	stdin_file->read = 1;
	add_to_list(process->open_files, stdin_file);
	
	open_file *stdout_file = (open_file *)malloc(sizeof(open_file));
	stdout_file->fd = stdout;
	stdout_file->used = 1;
	stdout_file->file_offset = 0;
	stdout_file->write = 1;
	stdout_file->read = 0;
	add_to_list(process->open_files, stdout_file);

	identity_page(process->page_table, (void *)KERNEL_BASE, KERNEL_LIMIT); //allocate and write process memory
	identity_page(process->page_table, (void *)0x0, 0xfffff);
	allocate_memory(process->page_table, (void *)PROCESS_CODE_BASE, length);
	write_virtual_memory(process->page_table, code, (void *)PROCESS_CODE_BASE, length);

	int arg_size = 0; //handle arguments
	int i;
	for(i = 0; i < argc; ++i)
		arg_size += strlen(argv[i]) + 1;
	arg_size += (argc+2)*4;
	process->regs->esp = 0x9fffff - arg_size;

	allocate_memory(process->page_table, (void *)(0x9fffff - arg_size), arg_size);
	int *p_argc = (int *)malloc(sizeof(int));
	*p_argc = argc;
	write_virtual_memory(process->page_table, (void *)p_argc, (void *)(0x9fffff - arg_size), sizeof(int)); 

	char **_argv = (char **)malloc(sizeof(char *)*argc);
	char *current_arg = (char *)(0x9fffff - arg_size + (argc + 2)*sizeof(int));
	for(i = 0; i < argc; ++i){
		write_virtual_memory(process->page_table, (void *)argv[i], (void *)current_arg, strlen(argv[i]) + 1); 
		_argv[i] = current_arg;
		current_arg += strlen(argv[i]) + 1;
	}
	write_virtual_memory(process->page_table, (void *)_argv, (void *)(0x9fffff - arg_size + 8), 4*argc); 
	*p_argc = 0x9fffff - arg_size + 8;
	write_virtual_memory(process->page_table, (void *)(p_argc), (void *)(0x9fffff - arg_size + 4), 4); 
	sti();
}

void exit_process(int result_code){ //exit process, clean resorces and run parent
	cli();
	print("process with PID ");
	print(itoa(current_process));
	print(" exited with result code ");
	print(itoa(result_code));
	print("\n");
	process_descriptor *process = (process_descriptor *)get_list_element(process_list, current_process);
	switch_memory_map(KERNEL_PAGE_TABLE);
	free_page_table(process->page_table);
	list_node *current_open_file_node = process->open_files->first;
	int fd = 0;
	while(current_open_file_node){ //close open files
		if(((open_file *)current_open_file_node->value)->used)
			fclose(fd);		

		free(current_open_file_node);
		current_open_file_node = current_open_file_node->next;
		++fd;
	}
	free(process->open_files);
	if(process->parent && process->parent->state == BLOCKED){ //resume parent process
		print("resuming parent process\n");
		process->parent->state = WAITING;
	}
	
	//TODO clean other stuff up too lazy right now :(
	remove_from_list(process_list, process);
	process->state = EXITED;
	current_process = -1;
	sti_forced();

	while(1) asm("hlt");
}

PID get_current_process(void){ //return current process PID
	return (PID)current_process;
}

/*int get_process_screen_index(PID process_index){ //return process's screen index
	return ((process_descriptor *)get_list_element(process_list, process_index))->screen_index;
}*/

void execute_from_process(char *file_name, int argc, char **argv){
	cli();
	if(argc != 0){ //handle arguments
		char **_argv = (char **)malloc(sizeof(char *)*argc);
		int i;
		for(i = 0; i < argc; ++i){
			_argv[i] = (char *)malloc(strlen(argv[i]) + 1);
			strcpy(_argv[i], argv[i]);
			_argv[i][strlen(argv[i])] = '\0';
		}
		argv = _argv;
	}else
		argv = 0;
	char *_file_name = (char *)malloc(strlen(file_name) + 1);
	strcpy(_file_name, file_name);
	file_name = _file_name;
	switch_memory_map(KERNEL_PAGE_TABLE);
	process_descriptor *process = (process_descriptor *)get_list_element(process_list, current_process);
	open_file *stdin = (open_file *)get_list_element(process->open_files, 0);
	open_file *stdout = (open_file *)get_list_element(process->open_files, 1);
	if(execute(file_name, stdin->fd, stdout->fd, argc, argv) == -1){
		sti();
		return;
	}
	process->state = BLOCKED; //execute
	process_descriptor *created_process = (process_descriptor *)get_list_element(process_list, process_list->length - 1);
	created_process->parent = process;
	created_process->ruid = process->euid;
	created_process->euid = process->euid;
	open_file *stdin_file = (open_file *)get_list_element(created_process->open_files, 0);
	stdin_file->file_offset = stdin->file_offset; 
	open_file *stdout_file = (open_file *)get_list_element(created_process->open_files, 1);
	stdout_file->file_offset = stdout->file_offset; 
	sti_forced();
	while(process->state == BLOCKED) 
		asm("hlt");
}

int seteuid(int new_euid){ //change effective uid
	process_descriptor *process = (process_descriptor *)get_list_element(process_list, current_process);
	if(process->ruid == 0){
		process->euid = new_euid;
		return 0;
	}

	return -1;
}

int get_current_euid(void){ //get effective uid
	if(current_process == -1)
		return 0;

	process_descriptor *process = (process_descriptor *)get_list_element(process_list, current_process);
	return process->euid;
}
