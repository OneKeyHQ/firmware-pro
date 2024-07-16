#ifndef _TLV_H_
#define _TLV_H_

#include <stdbool.h>
#include <stdint.h>

typedef struct {
  uint16_t tag;
  uint16_t length;
  uint8_t *value;
} tlv_t;

typedef struct {
  uint16_t len;
  uint8_t *value;
} tlv_value_t;

#define TLV_VAR(tag) tlv_t tlv_##tag
#define TLV_FIELD(tag) tlv_value_t lv_##tag

bool tlv_parse_tag(const uint8_t *data, uint8_t *end, uint16_t *tag,
                   uint16_t *offset);
bool tlv_parse_length(const uint8_t *data, uint8_t *end, uint16_t *length,
                      uint16_t *offset);

#endif
