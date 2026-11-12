#ifndef MAYHEM_FAKE_HC_H
#define MAYHEM_FAKE_HC_H

#include <stdint.h>
#include "usbh_core.h"

/* defined in core/usbh_core.c, not exported by usbh_core.h */
int usbh_enumerate(struct usbh_hubport *hport);
void usbh_hubport_release(struct usbh_hubport *hport);

void mayhem_host_register_classes(void);
struct usbh_hubport *mayhem_host_enumerate(uint8_t speed, const uint8_t *dev, const uint8_t *cfg, uint32_t cfg_len,
                                           const uint8_t *extra, uint32_t extra_len, int *ret);
void mayhem_host_release(struct usbh_hubport *hport);

#endif
