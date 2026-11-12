/*
 * Drives usbh_enumerate() against a fake host controller. The fuzz input is the whole device:
 *   byte 0        speed selector
 *   bytes 1..18   device descriptor
 *   bytes 19..20  configuration blob length (LE), then that many bytes of configuration descriptors
 *   remainder     answers to every other IN transfer (class requests, bulk and interrupt reads)
 * Class drivers for CDC-ACM/serial, HID and MSC are registered, so interface descriptors reach their connect().
 */
#include <stddef.h>
#include <stdint.h>
#include "fake_hc.h"

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    struct usbh_hubport *hport;
    uint32_t cfg_len;
    int ret;

    mayhem_host_register_classes();
    if (size < 21) {
        return -1;
    }
    cfg_len = (uint32_t)data[19] | ((uint32_t)data[20] << 8);
    if (21 + cfg_len > size) {
        return -1;
    }
    hport = mayhem_host_enumerate(1 + (data[0] % 6), data + 1, data + 21, cfg_len,
                                  data + 21 + cfg_len, (uint32_t)(size - 21 - cfg_len), &ret);
    mayhem_host_release(hport);
    return 0;
}
