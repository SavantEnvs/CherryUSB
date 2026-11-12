#ifndef MAYHEM_FAKE_DC_H
#define MAYHEM_FAKE_DC_H

#include <stdint.h>

void mayhem_device_init(void);
void mayhem_device_reset(void);
void mayhem_device_setup(const uint8_t setup[8]);
void mayhem_device_out(uint8_t ep, const uint8_t *data, uint32_t len);
void mayhem_device_in_done(uint8_t ep, uint32_t nbytes);
uint32_t mayhem_device_pending_write(uint8_t ep);

const uint8_t *mayhem_device_written(uint8_t ep);

#endif
