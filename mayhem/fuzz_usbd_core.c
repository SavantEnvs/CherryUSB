/*
 * Drives the device stack (usbd_core + CDC-ACM, MSC and HID classes) through a fake device controller.
 * The input is a stream of bus events:
 *   0 + 8 bytes          SETUP packet on EP0
 *   1 + ep + len16 + ..  OUT transfer completes with `len16` payload bytes on endpoint `ep`
 *   2 + ep + len16       IN transfer completes with `len16` bytes sent on endpoint `ep`
 *   3                    bus reset
 */
#include <stddef.h>
#include <stdint.h>
#include "fake_dc.h"

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    size_t pos = 0;

    if (size < 1) {
        return -1;
    }
    mayhem_device_init();
    mayhem_device_reset();

    while (pos < size) {
        uint8_t op = data[pos++] & 3;

        if (op == 0) {
            if (size - pos < 8) {
                break;
            }
            mayhem_device_setup(data + pos);
            pos += 8;
        } else if (op == 1) {
            uint32_t len;
            if (size - pos < 3) {
                break;
            }
            len = data[pos + 1] | ((uint32_t)data[pos + 2] << 8);
            if (len > size - pos - 3) {
                len = (uint32_t)(size - pos - 3);
            }
            mayhem_device_out(data[pos], data + pos + 3, len);
            pos += 3 + len;
        } else if (op == 2) {
            if (size - pos < 3) {
                break;
            }
            mayhem_device_in_done(data[pos], data[pos + 1] | ((uint32_t)data[pos + 2] << 8));
            pos += 3;
        } else {
            mayhem_device_reset();
        }
    }
    return 0;
}
