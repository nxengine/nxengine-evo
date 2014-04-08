
#ifndef _BUFIO_H
#define _BUFIO_H
#include "DBuffer.h"

uint8_t read_U8(const uint8_t **data, const uint8_t *data_end);
uint16_t read_U16(const uint8_t **data, const uint8_t *data_end);
uint32_t read_U32(const uint8_t **data, const uint8_t *data_end);
void write_U8(DBuffer *buffer, uint8_t data);
void write_U16(DBuffer *buffer, uint16_t data);
void write_U32(DBuffer *buffer, uint32_t data);
void write_U64(DBuffer *buffer, uint64_t data);
void write_F32(DBuffer *buffer, float data);
void write_F64(DBuffer *buffer, double data);
uint32_t read_U24(const uint8_t **data, const uint8_t *data_end);
void write_U24(DBuffer *buffer, uint32_t data);

#endif
