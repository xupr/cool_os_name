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
	registers *regs;
} process_descriptor;

typedef struct{
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
static int current_process = 0;
//void (*jump_to_ring3)(int offset);
extern void jump_to_ring3(int offset);
static int screen_index_for_shell = 0;

void init_process(void){
	char gdt[6];
	asm("sgdt %0" : "=m"(gdt));
	char *c = *((char **)(gdt + 2)) + 5*8;
	*(short *)(c+2) = (short)((int)tss & 0xFFFF);
	*(c+4) = (char)(((int)tss>>16) & 0xFF);
	*(c+7) = (char)(((int)tss>>24) & 0xFF);
	//print(itoa((int)tss));
	*(int *)(tss + 4) = 0x4fffff;
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

int pit_interrupt_handler(registers *regs){
	//print("kappa123");
	//registers *regs = regs_end+1;
	list_node *process_node = process_list->first;
	process_descriptor *process = (process_descriptor *)process_node->value;
	if(process_list->length == 0){
		execute("shell.o", screen_index_for_shell++);
		send_EOI(0);
		return 0;			
	}else if(process_list->length == 1){
		if(process->state == CREATED){
			print("running created process\n");
			process->state = RUNNING;
			process->quantum = 5;
			regs->esp = 0x9FFFFF;
			regs->cs = 0x1B;
			regs->ss = 0x23;
			regs->eip = PROCESS_CODE_BASE;
			//print(itoa(regs->eflags));
			switch_memory_map(process->page_table);
			send_EOI(0);
			return PROCESS_CODE_BASE;
		}
		
		//print(itoa(regs->eip));
		if(process->quantum > 1)
			--process->quantum;

		//execute("shell.o", screen_index_for_shell++);
		
		//execute("shell.o", screen_index_for_shell++);
		send_EOI(0);
		return 0;
	}

	while(process_node){
		process = (process_descriptor *)process_node->value;
		if(process->state == RUNNING){
		/*	print("running process quantum: ");
			print(itoa(process->quantum));
			print("\n");*/
			if(!(--process->quantum)){
				process->quantum = 5;
				//print("process quantum ended");
			}
			break;
		}
		
		process_node = process_node->next;
	}

	send_EOI(0);
	return 0;
}

FILE fopen(char *file_name){
	FILE fd = open(file_name);
	process_descriptor *process = (process_descriptor *)get_list_element(process_list, current_process);
	FILE vfd = process->open_files->length;
	open_file *file = (open_file *)malloc(sizeof(open_file));
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
	process->heap_start = (void *)(PROCESS_CODE_BASE + length);
	process->open_files = create_list();
	process->state = CREATED;
	process->screen_index = screen_index;
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
	return (PID)0;
}

int get_process_screen_index(PID process_index){
	return ((process_descriptor *)get_list_element(process_list, process_index))->screen_index;
}
