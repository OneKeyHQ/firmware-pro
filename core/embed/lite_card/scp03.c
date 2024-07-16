
#include <string.h>

#include "aes/aes.h"
#include "scp03.h"

#define SWAP_UINT32(x)                                     \
  (((x) >> 24) & 0x000000FF) | (((x) >> 8) & 0x0000FF00) | \
      (((x) << 8) & 0x00FF0000) | (((x) << 24) & 0xFF000000)

void scp03_generate_icv(uint8_t *aes_key, scp03_context *scp03_ctx, bool send) {
  aes_encrypt_ctx ctxe;
  uint8_t iv[AES_BLOCK_SIZE] = {0};
  uint8_t counter[AES_BLOCK_SIZE] = {0};
  uint32_t counter_temp;

  aes_init();

  if (send) {
    counter_temp = SWAP_UINT32(scp03_ctx->counter);
    memcpy(counter + 12, &counter_temp, 4);
  } else {
    scp03_ctx->counter++;
    counter_temp = SWAP_UINT32(scp03_ctx->counter);
    memcpy(counter + 12, &counter_temp, 4);
    counter[0] = 0x80;
  }
  aes_encrypt_key128(aes_key, &ctxe);

  aes_cbc_encrypt(counter, scp03_ctx->icv, AES_BLOCK_SIZE, iv, &ctxe);

  return;
}

void scp03_encrypt(uint8_t *aes_key, uint8_t *icv, uint8_t *data,
                   uint16_t data_len, uint8_t *output, uint16_t *output_len) {
  aes_encrypt_ctx ctxe;

  uint8_t pad_len = AES_BLOCK_SIZE - (data_len % AES_BLOCK_SIZE);
  memset(data + data_len, 0, pad_len);
  data[data_len] = 0x80;

  data_len += pad_len;

  aes_encrypt_key128(aes_key, &ctxe);

  aes_cbc_encrypt(data, output, data_len, icv, &ctxe);

  *output_len = data_len;

  return;
}

bool scp03_decrypt(uint8_t *aes_key, uint8_t *icv, uint8_t *data,
                   uint16_t data_len, uint8_t *output, uint16_t *output_len) {
  aes_decrypt_ctx ctxd;

  aes_decrypt_key128(aes_key, &ctxd);

  aes_cbc_decrypt(data, output, data_len, icv, &ctxd);

  for (uint8_t i = 0; i < 16; i++) {
    if (output[data_len - 1 - i] == 0x80) {
      *output_len = data_len - i - 1;
      return true;
    } else if (output[data_len - 1 - i] != 0) {
      *output_len = 0;
      return false;
    }
  }

  return false;
}

void scp03_init(scp03_context *scp03_ctx, uint8_t *mac_chain_value) {
  memset(scp03_ctx, 0, sizeof(scp03_context));

  scp03_ctx->counter = 1;

  memcpy(scp03_ctx->mac_chain_value, mac_chain_value,
         sizeof(scp03_ctx->mac_chain_value));
}