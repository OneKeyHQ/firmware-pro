#ifndef _JPEG_DMA_H_
#define _JPEG_DMA_H_

enum { JPEG_FILE_LVGL = 0, JPEG_FILE_FATFS };

void jpeg_init(void);
int jped_decode(char *path, uint32_t address);
void jpeg_decode_init(uint32_t address);
void jpeg_decode_file_operation(uint8_t mode);
int jpeg_get_decode_state(void);
int jpeg_get_decode_error(void);
int jpeg_decode_start(const char *path);
void jpeg_decode_info(uint32_t *width, uint32_t *height, uint32_t *subsampling);
#endif