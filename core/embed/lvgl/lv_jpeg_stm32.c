/**
 * @file lv_jpeg.c
 *
 */

/*********************
 *      INCLUDES
 *********************/
#include "lvgl.h"
#if 1
#include STM32_HAL_H

#include "jpeg_dma.h"
#include "mipi_lcd.h"

/*********************
 *      DEFINES
 *********************/

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 *  STATIC PROTOTYPES
 **********************/
static lv_res_t decoder_info(struct _lv_img_decoder_t *decoder, const void *src,
                             lv_img_header_t *header);
static lv_res_t decoder_open(lv_img_decoder_t *dec, lv_img_decoder_dsc_t *dsc);
static void decoder_close(lv_img_decoder_t *dec, lv_img_decoder_dsc_t *dsc);

/**********************
 *  STATIC VARIABLES
 **********************/

// static lv_fs_file_t f;

/**********************
 *      MACROS
 **********************/

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

/**
 * Register the PNG decoder functions in LVGL
 */
void lv_st_jpeg_init(void) {
  jpeg_init();
  jpeg_decode_file_operation(JPEG_FILE_LVGL);
  lv_img_decoder_t *dec = lv_img_decoder_create();
  lv_img_decoder_set_info_cb(dec, decoder_info);
  lv_img_decoder_set_open_cb(dec, decoder_open);
  lv_img_decoder_set_close_cb(dec, decoder_close);
}

/**********************
 *   STATIC FUNCTIONS
 **********************/

#define LDB_WORD(ptr)                                 \
  (uint16_t)(((uint16_t) * ((uint8_t *)(ptr)) << 8) | \
             (uint16_t) * (uint8_t *)((ptr) + 1))

static uint32_t get_jpeg_info(const char *fn, uint16_t *w, uint16_t *h) {
  uint8_t buf[64];
  uint32_t len;

  uint16_t marker, marker_len;

  lv_fs_file_t file;
  lv_fs_res_t res = lv_fs_open(&file, fn, LV_FS_MODE_RD);
  if (res != LV_FS_RES_OK) {
    return LV_RES_INV;
  }

  if (lv_fs_read(&file, buf, 2, &len) == LV_FS_RES_OK) {
    marker = LDB_WORD(buf);
    if (marker != 0xFFD8) {
      return LV_RES_INV;
    }

    while (1) {
      if (lv_fs_read(&file, buf, 4, &len) != LV_FS_RES_OK) {
        break;
      }
      if (len == 0) {
        break;
      }
      if (buf[0] != 0xFF) {
        break;
      }
      switch (buf[1]) {
        case 0xC0:
          lv_fs_read(&file, buf, 16, &len);
          *h = LDB_WORD(buf + 1);
          *w = LDB_WORD(buf + 3);

          lv_fs_close(&file);
          return LV_RES_OK;
          break;
        case 0xC1: /* SOF1 */
        case 0xC2: /* SOF2 */
        case 0xC3: /* SOF3 */
        case 0xC5: /* SOF5 */
        case 0xC6: /* SOF6 */
        case 0xC7: /* SOF7 */
        case 0xC9: /* SOF9 */
        case 0xCA: /* SOF10 */
        case 0xCB: /* SOF11 */
        case 0xCD: /* SOF13 */
        case 0xCE: /* SOF14 */
        case 0xCF: /* SOF15 */
          lv_fs_close(&file);
          return LV_RES_INV;
        default:
          marker_len = LDB_WORD(buf + 2);
          lv_fs_seek(&file, marker_len - 2, LV_FS_SEEK_CUR);
          break;
      }
    }
  }
  lv_fs_close(&file);
  return LV_RES_INV;
}

/**
 * Get info about a PNG image
 * @param src can be file name or pointer to a C array
 * @param header store the info here
 * @return LV_RES_OK: no error; LV_RES_INV: can't get the info
 */
static lv_res_t decoder_info(struct _lv_img_decoder_t *decoder, const void *src,
                             lv_img_header_t *header) {
  (void)decoder; /*Unused*/

  uint16_t w = 0, h = 0;

  lv_img_src_t src_type = lv_img_src_get_type(src); /*Get the source type*/

  /*If it's a PNG file...*/
  if (src_type == LV_IMG_SRC_FILE) {
    const char *fn = src;
    if (strncmp(lv_fs_get_ext(fn), "jpg", strlen("jpg")) == 0 ||
        strncmp(lv_fs_get_ext(fn), "jpeg", strlen("jpeg")) == 0) {
      /*Check the extension*/

      if (get_jpeg_info(fn, &w, &h) == LV_RES_OK) {
        /*Save the data in the header*/
        header->always_zero = 0;
        header->cf = LV_IMG_CF_TRUE_COLOR;
        /*The width and height are stored in Big endian format so convert
        them to
         * little endian*/
        header->w = w;
        header->h = h;

        return LV_RES_OK;
      }
    }
  }

  return LV_RES_INV; /*If didn't succeed earlier then it's an error*/
}

/**
 * Open a PNG image and return the decided image
 * @param src can be file name or pointer to a C array
 * @param style style of the image object (unused now but certain formats might
 * use it)
 * @return pointer to the decoded image or `LV_IMG_DECODER_OPEN_FAIL` if failed
 */
static lv_res_t decoder_open(lv_img_decoder_t *decoder,
                             lv_img_decoder_dsc_t *dsc) {
  (void)decoder; /*Unused*/

  uint8_t *img_data = NULL;

  /*If it's a PNG file...*/
  if (dsc->src_type == LV_IMG_SRC_FILE) {
    const char *fn = dsc->src;
    if (strncmp(lv_fs_get_ext(fn), "jpg", strlen("jpg")) == 0 ||
        strncmp(lv_fs_get_ext(fn), "jpeg", strlen("jpeg")) == 0) {
      /*Check the extension*/
      jpeg_decode_init(FMC_SDRAM_JPEG_OUTPUT_DATA_BUFFER_ADDRESS);
      /* Start JPEG decoding with DMA method */
      if (jpeg_decode_start(fn) != 0) {
        return LV_RES_INV;
      }

      if (jpeg_get_decode_error() == 1) {
        return LV_RES_INV;
      }
      uint32_t width, height, subsampling;

      jpeg_decode_info(&width, &height, &subsampling);

      img_data = lodepng_malloc(width * height * 2);
      if (!img_data) {
        return LV_RES_INV;
      }
      dma2d_copy_ycbcr_to_rgb(
          (uint32_t *)FMC_SDRAM_JPEG_OUTPUT_DATA_BUFFER_ADDRESS,
          (uint32_t *)img_data, width, height, subsampling);

      // uint32_t addr = (uint32_t)img_data;
      // uint32_t end = addr + width * height * 2;

      // #define L1C_LINE_SIZE 32

      // uint32_t aligned_addr = addr & ~(L1C_LINE_SIZE - 1U);
      // uint32_t aligned_end = (end + L1C_LINE_SIZE - 1U) & ~(L1C_LINE_SIZE -
      // 1U); uint32_t aligned_size = aligned_end - aligned_addr;

      // SCB_InvalidateDCache_by_Addr((uint32_t *)aligned_addr, aligned_size);

      dsc->img_data = img_data;
      return LV_RES_OK; /*The image is fully decoded. Return with its pointer*/
    }
  }

  return LV_RES_INV; /*If not returned earlier then it failed*/
}

/**
 * Free the allocated resources
 */
static void decoder_close(lv_img_decoder_t *decoder,
                          lv_img_decoder_dsc_t *dsc) {
  LV_UNUSED(decoder); /*Unused*/
  if (dsc->img_data) {
    lodepng_free((uint8_t *)dsc->img_data);
    dsc->img_data = NULL;
  }
}

#endif
