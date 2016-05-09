#include "../c/headers/os.h"
#include "../c/headers/stdio.h"
#include "../c/headers/string.h"
#include "../c/headers/stat.h"

int main(int argc, char **argv){
	char *dir_name;
	int show_details = 0;
	int i;
	struct stat *st_buff;
	char *path;
	for(i = 1; i < argc; ++i){
		print("asd");
		print(argv[i]);
		if(!strcmp(argv[i], "-l"))
			show_details = 1;
		else
			dir_name = argv[i];
	}

	if(!dir_name)
		dir_name = "/";
	if(show_details){
 		st_buff = (struct stat *)malloc(sizeof(struct stat));
		path = (char *)malloc(128);
	}
	print(dir_name);

	DIR dd = opendir(dir_name);
	if(dd == -1){
		print("couldn't open the directory ");
		print(dir_name);
		print("\n");
		return 0;
	}

	char *file_name = (char *)malloc(128);
	while(readdir(file_name, 128, dd) != -1){
		if(show_details){
			memset(path, 0, 128);
			strcpy(path, dir_name);
			if(dir_name[strlen(dir_name) - 1] != '/')
				path[strlen(dir_name)] = '/';
			strcpy(path + strlen(path), file_name);
			print(path);
		}else{
			print(file_name);
			print(" ");
		}
	}
	print("\n");
	closedir(dd);

	return 0;
}
