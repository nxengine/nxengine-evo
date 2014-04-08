
#ifndef _BUFIO_H
#define _BUFIO_H
#include "DBuffer.h"

uint8_t read_U8(const uint8_t **data, const uint8_t *data_end);
uint16_t read_U16(const uint8_t **data, const uint8_t *data_end);
uint32_t read_U24(const uint8_t **data, const uint8_t *data_end);
uint32_t read_U32(const uint8_t **data, const uint8_t *data_end);

#endif
