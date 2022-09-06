//파일경로
#define BD_STAT_LENGH 16
#define READ_LENGH 16

void init_fileconfig(void);
int fs_read(void);
int fs_write(uint8_t id, int relay);
