#ifndef _PN532_STUB_H_
#define _PN532_STUB_H_

#include <stdint.h>
#include <stdbool.h>

typedef struct
{

    void (*init)(void);
    void (*chip_reset_ctl)(bool enable);

} pn532_stub_t;

pn532_stub_t* get_stub_controller(void);

#endif // _PN532_STUB_H_
