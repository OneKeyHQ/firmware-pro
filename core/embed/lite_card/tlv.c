#include "tlv.h"

bool tlv_parse_tag(const uint8_t *data, uint8_t *end, uint16_t *tag,
                   uint16_t *offset) {
  if ((*data & 0x1f) == 0x1f) {
    *tag = (*data << 8) | *(data + 1);
    *offset = 2;
  } else {
    *tag = *data;
    *offset = 1;
  }

  return true;
}

bool tlv_parse_length(const uint8_t *data, uint8_t *end, uint16_t *length,
                      uint16_t *offset) {
  *length = 0;
  if (*data & 0x80) {
    uint8_t len_bytes = *data & 0x7f;
    if (len_bytes > 2) {
      return false;
    }

    for (uint8_t i = 0; i < len_bytes; i++) {
      *length = (*length << 8) | *(data + 1 + i);
    }
    *offset = len_bytes + 1;
    if (data + *offset + *length > end) {
      return false;
    }
  } else {
    *length = *data;
    *offset = 1;
    if (data + *offset + *length > end) {
      return false;
    }
  }

  return true;
}
