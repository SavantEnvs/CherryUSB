/* Shadows common/usb_log.h: USB_ASSERT upstream spins forever, which a fuzzer sees as a timeout. */
#ifndef MAYHEM_USB_LOG_H
#define MAYHEM_USB_LOG_H

#include_next "usb_log.h"

#include <stdlib.h>

#undef USB_ASSERT
#undef USB_ASSERT_MSG
#define USB_ASSERT(f)                      \
    do {                                   \
        if (!(f)) {                        \
            abort();                       \
        }                                  \
    } while (0)
#define USB_ASSERT_MSG(f, fmt, ...)        \
    do {                                   \
        if (!(f)) {                        \
            abort();                       \
        }                                  \
    } while (0)

#endif
