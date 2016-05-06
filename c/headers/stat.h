typedef enum {
	NEW_FILE,
	REGULAR_FILE,
	DIRECTORY
} FILE_TYPE;

struct stat{
	FILE_TYPE type;
	unsigned short access;
	unsigned int creator_uid;
	unsigned int size;
	unsigned int creation_date;
	unsigned int update_date;
};
