#ifndef _PN532_SPI_
#define _PN532_SPI_

#include "spi.h"

typedef struct
{
    void (*spi_init)(void);
    void (*spi_deinit)(void);
    void (*chip_select)(bool enable);
    void (*write)(uint8_t* buf, uint32_t size);
    void (*read)(uint8_t* buf, uint32_t size);
} pn532_spi_t;

pn532_spi_t* get_spi_controller(void);

#endif
