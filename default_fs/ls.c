#include "../c/headers/os.h"
#include "../c/headers/stdio.h"

int main(int argc, char **argv){
	char *dir_name;
	if(argc < 2)
		dir_name = "/";
	else
		dir_name = argv[1];

	DIR dd = opendir(dir_name);
	if(dd == -1){
		print("couldn't open the directory ");
		print(dir_name);
		print("\n");
		return 0;
	}
	char *file_name = (char *)malloc(128);
	while(readdir(file_name, 128, dd) != -1){
		print(file_name);
		print(" ");
	}
	print("\n");
	closedir(dd);

	return 0;
}
