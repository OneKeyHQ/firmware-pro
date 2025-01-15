#include <string.h>

#include "camera_qrcode.h"
#include "quirc.h"
#include "zbar.h"
#include "camera.h"
#include "display.h"
#include "mipi_lcd.h"

static struct quirc* qr_decoder;

static inline uint8_t rgb565_to_gray(uint16_t rgb565)
{
    // Extract R, G, B values from RGB565
    int r = ((rgb565 >> 11) & 0x1F) << 3;
    int g = ((rgb565 >> 5) & 0x3F) << 2;
    int b = (rgb565 & 0x1F) << 3;

    // Convert RGB to grayscale using fast integer approximation
    int gray_value = (r * 76 + g * 150 + b * 29) >> 8;

    return gray_value & 0xff;
}

static uint8_t* gray_value = (uint8_t*)(CAM_BUF_ADDRESS);
static inline int zbar_decode_buffer(uint8_t* raw, uint8_t* out, uint32_t len)
{
    uint16_t* rgb565_value = (uint16_t*)raw;
    int qr_code_len = 0;

    for ( int i = 0; i < WIN_W * WIN_H / 4; i++ )
    {
        gray_value[4 * i] = rgb565_to_gray(rgb565_value[4 * i]);
        gray_value[4 * i + 1] = rgb565_to_gray(rgb565_value[4 * i + 1]);
        gray_value[4 * i + 2] = rgb565_to_gray(rgb565_value[4 * i + 2]);
        gray_value[4 * i + 3] = rgb565_to_gray(rgb565_value[4 * i + 3]);
    }

    zbar_image_scanner_t* scanner = NULL;
    /* create a reader */
    scanner = zbar_image_scanner_create();

    /* configure the reader */
    zbar_image_scanner_set_config(scanner, 0, ZBAR_CFG_ENABLE, 1);
    zbar_image_scanner_set_config(scanner, 0, ZBAR_CFG_X_DENSITY, 2);
    zbar_image_scanner_set_config(scanner, 0, ZBAR_CFG_Y_DENSITY, 2);

    /* wrap image data */
    zbar_image_t* image = zbar_image_create();
    zbar_image_set_format(image, *(int*)"Y800");
    zbar_image_set_size(image, WIN_W, WIN_H);
    zbar_image_set_data(image, (uint8_t*)gray_value, WIN_W * WIN_H, zbar_image_free_data);

    /* scan the image for barcodes */
    int n = zbar_scan_image(scanner, image);

    if ( n != 1 )
    {
        zbar_image_destroy(image);
        zbar_image_scanner_destroy(scanner);
        return 0;
    }
    /* extract results */
    const zbar_symbol_t* symbol = zbar_image_first_symbol(image);
    for ( ; symbol; symbol = zbar_symbol_next(symbol) )
    {
        /* do something useful with results */
        zbar_symbol_get_type(symbol);

        const char* data = zbar_symbol_get_data(symbol);
        qr_code_len = strlen(data);
        if ( len >= qr_code_len )
        {
            memcpy(out, data, qr_code_len);
        }
    }

    /* clean up */
    zbar_image_destroy(image);
    zbar_image_scanner_destroy(scanner);

    return qr_code_len;
}

int quirc_decode_buffer(uint8_t* buffer, uint8_t* out, uint32_t len)
{
    uint8_t* image_data;
    struct quirc_code code;
    struct quirc_data data = {0};
    uint16_t* rgb565_value = (uint16_t*)buffer;

    qr_decoder = quirc_new();

    if ( quirc_resize(qr_decoder, WIN_W, WIN_H) < 0 )
    {
        goto fail;
    }
    image_data = quirc_begin(qr_decoder, NULL, NULL);
    for ( int i = 0; i < WIN_W * WIN_H; i++ )
    {
        image_data[i] = rgb565_to_gray(rgb565_value[i]);
    }
    quirc_end(qr_decoder);

    int num_codes = quirc_count(qr_decoder);
    if ( num_codes != 1 )
    {
        goto fail;
    }
    quirc_extract(qr_decoder, 0, &code);
    quirc_decode_error_t err = quirc_decode(&code, &data);
    if ( err == QUIRC_ERROR_DATA_ECC )
    {
        quirc_flip(&code);
        err = quirc_decode(&code, &data);
    }
    if ( err )
    {
        goto fail;
    }
fail:
    quirc_destroy(qr_decoder);
    if ( len >= data.payload_len )
    {
        memcpy(out, data.payload, data.payload_len);
    }

    return data.payload_len;
}

int camera_qr_decode(uint32_t x, uint32_t y, uint8_t* data, uint32_t data_len)
{

    int len = 0;
    if ( !camera_is_online() )
    {
        return 0;
    }
    camera_capture_start();
    if ( camera_capture_done() )
    {
        dma2d_copy_buffer(
            (uint32_t*)CAM_BUF_ADDRESS, (uint32_t*)FMC_SDRAM_LTDC_BUFFER_ADDRESS, x, y, WIN_W, WIN_H
        );
        len = zbar_decode_buffer((uint8_t*)CAM_BUF_ADDRESS, data, data_len);
    }

    return len;
}

void camera_test(void)
{
    uint8_t qr_code[512];
    int len;

    display_clear();

    while ( 1 )
    {
        len = camera_qr_decode(80, 80, qr_code, sizeof(qr_code));
        if ( len )
        {
            display_bar(0, 600, 480, 50, COLOR_BLACK);
            display_text(0, 650, (char*)qr_code, 20, FONT_NORMAL, COLOR_WHITE, COLOR_BLACK);
        }
    }
}
