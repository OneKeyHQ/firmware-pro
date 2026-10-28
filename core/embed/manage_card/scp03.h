#ifndef _SCP03_H_
#define _SCP03_H_

#include <stdbool.h>
#include <stdint.h>

#define SCP03_MAC_SIZE 8

typedef struct {
  uint32_t counter;
  uint8_t icv[16];
  uint8_t mac_chain_value[16];
} scp03_context;
void scp03_generate_icv(uint8_t *aes_key, scp03_context *scp03_ctx, bool send);
void scp03_encrypt(uint8_t *aes_key, uint8_t *icv, uint8_t *data,
                   uint16_t data_len, uint8_t *output, uint16_t *output_len);
bool scp03_decrypt(uint8_t *aes_key, uint8_t *icv, uint8_t *data,
                   uint16_t data_len, uint8_t *output, uint16_t *output_len);
void scp03_init(scp03_context *scp03_ctx, uint8_t *mac_chain_value);

#endif
