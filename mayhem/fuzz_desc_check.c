/* Fuzzes the configuration-descriptor validator that gates every host enumeration and device registration. */
#include <stddef.h>
#include <stdint.h>
#include "usbh_core.h"
#include "usb_desc_check.h"

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    uint32_t err_offset;

    if (size < 1) {
        return -1;
    }
    usb_desc_check_config_desc(data[0], data + 1, (uint32_t)(size - 1), &err_offset);
    return 0;
}
