#include "../c/headers/stdlib.h"
#include "../c/headers/stdio.h"
#include "../c/headers/string.h"

int main(void){
	char *username = "a";
	char *password = "a";
	while(1){
			/*execute("shell.bin");*/
		print("username: ");
		char input_username[32];
		input(input_username, 32);
		print("password: ");
		char input_password[32];
		input(input_password, 32);
		if(!strcmp(username, input_password) && !strcmp(password, input_password))
			execute("shell.bin");
		else
			print("wrong\n");
	}
}
