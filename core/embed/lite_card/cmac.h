#ifndef __CMAC_H__
#define __CMAC_H__

void AES128_CMAC(unsigned char *key, unsigned char *input, int length,
                 unsigned char *mac);

#endif
