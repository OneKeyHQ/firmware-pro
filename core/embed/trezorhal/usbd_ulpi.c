#include STM32_HAL_H

#include "usbd_ulpi.h"

#define USBULPI_PHYCR ((uint32_t)(0x40040000 + 0x034))
#define USBULPI_D07 ((uint32_t)0x000000FF)
#define USBULPI_New ((uint32_t)0x02000000)
#define USBULPI_RW ((uint32_t)0x00400000)
#define USBULPI_S_BUSY ((uint32_t)0x04000000)
#define USBULPI_S_DONE ((uint32_t)0x08000000)

#define USB_OTG_READ_REG32(reg) (*(__IO uint32_t *)(reg))
#define USB_OTG_WRITE_REG32(reg, value) (*(__IO uint32_t *)(reg) = (value))

uint32_t USB_ULPI_Read(uint32_t Addr) {
  __IO uint32_t val = 0;
  __IO uint32_t timeout = 1000; /* Can be tuned based on the Clock or etc... */

  val = USB_OTG_READ_REG32(USBULPI_PHYCR);
  while ((val & USBULPI_S_BUSY) != 0 && (timeout--)) {
    val = USB_OTG_READ_REG32(USBULPI_PHYCR);
  }

  if (timeout == 0) {
    return 0;
  }

  timeout = 1000;

  USB_OTG_WRITE_REG32(USBULPI_PHYCR, USBULPI_New | (Addr << 16));

  val = USB_OTG_READ_REG32(USBULPI_PHYCR);
  while (((val & USBULPI_S_DONE) == 0) && (timeout--)) {
    val = USB_OTG_READ_REG32(USBULPI_PHYCR);
  }
  val = USB_OTG_READ_REG32(USBULPI_PHYCR);
  return (val & 0x000000ff);
}

/**
 * @brief  Write CR value
 * @param  Addr the Address of the ULPI Register
 * @param  Data Data to write
 * @retval Returns value of PHY CR register
 */
uint32_t USB_ULPI_Write(uint32_t Addr,
                        uint32_t Data) /* Parameter is the Address of the ULPI
                                          Register & Date to write */
{
  __IO uint32_t val;
  __IO uint32_t timeout = 100; /* Can be tuned based on the Clock or etc... */
  // __IO uint32_t bussy = 0;

  USB_OTG_WRITE_REG32(USBULPI_PHYCR, USBULPI_New | USBULPI_RW | (Addr << 16) |
                                         (Data & 0x000000ff));
  val = USB_OTG_READ_REG32(USBULPI_PHYCR);
  while (((val & USBULPI_S_DONE) == 0) && (timeout--)) {
    val = USB_OTG_READ_REG32(USBULPI_PHYCR);
    // bussy = val & USBULPI_S_BUSY;
  }

  val = USB_OTG_READ_REG32(USBULPI_PHYCR);
  return 0;
}
