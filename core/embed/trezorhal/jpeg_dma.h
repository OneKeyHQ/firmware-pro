#ifndef _JPEG_DMA_H_
#define _JPEG_DMA_H_

#include <stdbool.h>

enum { JPEG_FILE_LVGL = 0, JPEG_FILE_FATFS };

void jpeg_init(void);
int jped_decode(char *path, uint32_t address);
int jped_decode_to_address(char *path, uint32_t src_address,
                           uint32_t dst_address);
void jpeg_decode_init(uint32_t address);
void jpeg_decode_file_operation(uint8_t mode);
int jpeg_get_decode_state(void);
int jpeg_get_decode_error(void);
int jpeg_decode_start(const char *path);
void jpeg_decode_info(uint32_t *width, uint32_t *height, uint32_t *subsampling);

void jpeg_save_state(void);
void jpeg_restore_state(void);

#endif