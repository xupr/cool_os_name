#include "../headers/filesystem.h"
#include "../headers/ata.h"

#define SECTOR_SIZE 512

int sd_read(char *buff, int count, file_descriptor *fd){
	int start_sector = fd->file_offset/SECTOR_SIZE;
	int end_sector = (fd->file_offset + count)/SECTOR_SIZE;
	int start_offset = fd->file_offset%SECTOR_SIZE;
	int end_offset = (fd->file_offset + count)%SECTOR_SIZE;
	if(end_sector - start_sector)
		ata_read_sectors(start_sector, end_sector - start_sector, buff + SECTOR_SIZE - start_offset);

	if(start_offset || end_offset){
		char *_buff = (char *)malloc(SECTOR_SIZE);	
		if(start_sector != end_sector){
			if(start_offset){
				ata_read_sectors(start_sector, 1, _buff);
				memcpy(buff, _buff + start_offset, SECTOR_SIZE - start_offset);
			}

			if(end_offset){
				ata_read_sectors(end_sector, 1, _buff);
				memcpy(buff + start_offset + (end_sector - start_sector)*SECTOR_SIZE, _buff, end_offset);
			}
		}else{
			ata_read_sectors(start_sector, 1 , _buff);
			memcpy(buff, _buff + start_offset, end_offset - start_offset);
		}

		free(_buff);
	}

	fd->file_offset += count;
	return count;
}

int sd_write(char *buff, int count, file_descriptor *fd){
	int start_sector = fd->file_offset/SECTOR_SIZE;
	int end_sector = (fd->file_offset + count)/SECTOR_SIZE;
	int start_offset = fd->file_offset%SECTOR_SIZE;
	int end_offset = (fd->file_offset + count)%SECTOR_SIZE;
	if(end_sector - start_sector)
		ata_write_sectors(start_sector, end_sector - start_sector, buff + SECTOR_SIZE - start_offset);

	if(start_offset || end_offset){
		char *_buff = (char *)malloc(SECTOR_SIZE);	
		if(start_sector != end_sector){
			if(start_offset){
				ata_read_sectors(start_sector, 1, _buff);
				memcpy(_buff + start_offset, buff, SECTOR_SIZE - start_offset);
				ata_write_sectors(start_sector, 1, _buff);
			}

			if(end_offset){
				ata_read_sectors(end_sector, 1, _buff);
				memcpy(buff + start_offset + (end_sector - start_sector)*SECTOR_SIZE, _buff, end_offset);
				ata_write_sectors(end_sector, 1, _buff);
			}
		}else{
			ata_read_sectors(start_sector, 1 , _buff);
			memcpy(_buff + start_offset, buff, end_offset - start_offset);
			ata_write_sectors(start_sector, 1, _buff);
		}

		free(_buff);
	}

	fd->file_offset += count;
	return count;
}

void sd_seek(file_descriptor *fd, int new_offset){
	fd->file_offset = new_offset;
}

void init_sd(){
	cli();
	inode *ind;
	if(!(ind = get_inode("/dev/sda"))){
		close(open("/dev/sda", "r"));
		ind = get_inode("/dev/sda");
	}
	ind->type = SPECIAL_FILE;
	ind->major = SD;
	ind->access = 0666;
	special_file_methods *sf_methods = (special_file_methods *)malloc(sizeof(special_file_methods));
	sf_methods->open = 0;
	sf_methods->read = sd_read;
	sf_methods->write = sd_write;
	sf_methods->close = 0;
	sf_methods->seek = sd_seek;
	add_special_file_method(SD, sf_methods);	
	sti();
}
