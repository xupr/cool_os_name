#include "c/headers/stdlib.h"
#include "c/headers/stdio.h"
#include "c/headers/string.h"

int main(void){
	char write_file_commad[] = "write";

//	while(1){
		print(">>> ");
		char str[] = "write asd.txt hello world!";
		//input(str, 32);
		
		char *command = strtok(str, " \n");
		if(!strcmp(command, write_file_commad)){
			print("holy");
			char *file_name = strtok(0, " \n"),
			     *data = strtok(0, " \n");
			print(file_name);
			FILE fd = open(file_name);
			//write(fd, data, strlen(data));
		}	
		//	print("i'm so fking happy right now!");
//	}
/*	char *str = "safta is love i said";
//	print(str);
	char *token = strtok(str, " ");
	while(token){
		print(token);
		print("\n");
		token = strtok(0, " ");
	}*/
	return 0;
}
