void init_ata(void);
void ata_read_sectors(int lba, char sector_count, char *buffer);
void ata_write_sectors(int lba, char sector_count, char *buffer);
unsigned int ata_get_sector_count(void);
