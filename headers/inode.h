typedef enum {
	REGULAR_FILE,
	DIRECTORY
} FILE_TYPE;

typedef struct {
	unsigned char bound;
	FILE_TYPE type;
	unsigned short access;
	unsigned int creator_uid;
	unsigned int size;
	unsigned int address_block;
	unsigned short name_address;
	unsigned int creation_date;
	unsigned int update_date;
} inode;
