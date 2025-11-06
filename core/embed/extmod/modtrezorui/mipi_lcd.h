#ifndef _MIPI_LCD_H_
#define _MIPI_LCD_H_

#include <stdint.h>

#include "sdram.h"

// #ifdef __cplusplus
// extern "C" {
// #endif

#define DISPLAY_MEMORY_BASE FMC_SDRAM_LTDC_BUFFER_ADDRESS

/* Back-light control pin */
#define LCD_BL_CTRL_PIN GPIO_PIN_0
#define LCD_BL_CTRL_GPIO_PORT GPIOK

/* LCD reset pin */
#define LCD_RESET_PIN GPIO_PIN_3
#define LCD_RESET_GPIO_PORT GPIOG

/* LCD tearing effect pin */
#define LCD_TE_PIN GPIO_PIN_2
#define LCD_TE_GPIO_PORT GPIOJ

typedef struct {
  uint32_t hres;
  uint32_t vres;
  uint32_t hsync;
  uint32_t hbp;
  uint32_t hfp;
  uint32_t vsync;
  uint32_t vbp;
  uint32_t vfp;

  uint32_t pixel_format_ltdc;
  uint32_t pixel_format_dsi;
  uint32_t bbp;
  uint32_t fb_base;

  struct {
    uint32_t PLL3N;
    uint32_t PLL3R;
    uint32_t PLL3FRACN;
  } ltdc_pll;

} DisplayParam_t;

typedef struct {
  uint32_t x0;
  uint32_t x1;
  uint32_t y0;
  uint32_t y1;
  uint32_t pixel_format;
  uint32_t address;
} LTDC_LAYERCONFIG;

extern float lcd_fps;
extern const DisplayParam_t lcd_params;

typedef int (*DbgPrintf_t)(const char *fmt, ...);
void disp_set_dbg_printf(DbgPrintf_t func);

int DSI_DCS_read(uint16_t cmd, uint8_t *data, uint16_t data_len);
int DSI_DCS_write(uint16_t cmd, uint8_t *data, uint16_t data_len);

void lcd_pwm_init(void);
void lcd_init(void);

void lcd_ltdc_dsi_disable(void);
void lcd_ltdc_dsi_enable(void);

void lcd_refresh_suspend(void);
void lcd_refresh_resume(void);

int lcd_ltdc_busy(void);

void display_fp(uint16_t x, uint16_t y, uint16_t w, uint16_t h,
                const uint8_t *data);

void fb_write_pixel(uint32_t x_pos, uint32_t y_pos, uint32_t color);
void fb_fill_rect(uint32_t x_pos, uint32_t y_pos, uint32_t width,
                  uint32_t height, uint32_t color);
void fb_draw_hline(uint32_t x_pos, uint32_t y_pos, uint32_t len,
                   uint32_t color);
void fb_draw_vline(uint32_t x_pos, uint32_t y_pos, uint32_t len,
                   uint32_t color);
void dma2d_copy_buffer(uint32_t *pSrc, uint32_t *pDst, uint16_t x, uint16_t y,
                       uint16_t xsize, uint16_t ysize);
void dma2d_copy_ycbcr_to_rgb(uint32_t *pSrc, uint32_t *pDst, uint16_t xsize,
                             uint16_t ysize, uint32_t ChromaSampling,
                             uint16_t output_line_width);

void lcd_set_src_addr(uint32_t addr);
uint32_t lcd_get_src_addr(void);

// Second layer functions
void lcd_add_second_layer(void);
// void lcd_clear_layer2_memory(void);
void lcd_ensure_second_layer(void);
void lcd_animation_init(void);

// CoverBackground hardware layer functions - simplified version (transparent status bar feature removed)
void lcd_cover_background_init(void);
void lcd_cover_background_show(void);
void lcd_cover_background_hide(void);
void lcd_cover_background_set_visible(bool visible);
void lcd_cover_background_set_image(const void* image_data, uint32_t image_size);
void lcd_cover_background_load_jpeg(const char* jpeg_path);
void lcd_cover_background_move_to_y(int16_t y_position);
void lcd_cover_background_animate_to_y(int16_t target_y, uint16_t duration_ms);
bool lcd_cover_background_is_visible(void);
bool lcd_cover_background_update_animation(void);
bool lcd_cover_background_is_animating(void);
bool lcd_cover_background_has_active_animation(void);
void lcd_cover_background_stop_animation(void);
void lcd_cover_background_start_animation(int16_t target_y, uint16_t duration_ms);
uint8_t lcd_cover_background_get_opacity(void);

// #ifdef __cplusplus
// }
// #endif

#endif
