#ifndef _USBD_ULPI_H_
#define _USBD_ULPI_H_

#define ULPI_REGISTER_VENDOR_ID 0x00
#define ULPI_REGISTER_FUNCTION_CONTROL_READ 0x04
#define ULPI_REGISTER_FUNCTION_CONTROL_WRITE 0x05
#define ULPI_REGISTER_OTG_CONTROL 0x0A
#define ULPI_REGISTER_IRQ_STATUS 0x13
#define ULPI_REGISTER_LINESTATE 0x15
#define ULPI_REGISTER_IO_POWER 0x39

uint32_t USB_ULPI_Read(uint32_t Addr);
uint32_t USB_ULPI_Write(uint32_t Addr, uint32_t Data);

#endif
