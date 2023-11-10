#ifndef _GT911_H_
#define _GT911_H_

#define GT911

#define GT911_ADDR (0x5D << 1)
#define GTP_REG_VERSION 0x8140
#define GTP_READ_COOR_ADDR 0x814E
#define GTP_READ_POINT_ADDR 0x8158

void gt911_init(void);
uint32_t gt911_read_location(void);

#endif
