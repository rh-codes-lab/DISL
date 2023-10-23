#pragma once
#ifdef __cplusplus
extern "C" {
#endif
int init_sd_arduino(void);
void deinit_sd_arduino(void);
void init_spi_sd_fs(void);
void init_external_sd_fs(void);
void list_dir(char *path, char *outBuff);
void listDir(char *path);
int remove_file(char *filename);
#ifdef __cplusplus
}
#endif
