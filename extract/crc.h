#ifndef __CRC_H_
#define __CRC_H_

void crc_init(void);
uint32_t crc_calc(uint8_t *buf, uint32_t size);
uint32_t reflect(uint32_t value, int size);
#endif