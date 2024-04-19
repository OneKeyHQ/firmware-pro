#ifndef _GT911_H_
#define _GT911_H_

#define GT911

#define GT911_ADDR (0x5D << 1)
#define GTP_REG_VERSION 0x8140
#define GTP_READ_COOR_ADDR 0x814E
#define GTP_READ_POINT_ADDR 0x8158
#define GTP_REG_SLEEP 0x8040
#define GTP_REG_MODSWITCH1 0x804D

void gt911_init(void);
uint32_t gt911_read_location(void);
void gt911_enter_sleep(void);
void gt911_enable_irq(void);
void gt911_disable_irq(void);

#endif
