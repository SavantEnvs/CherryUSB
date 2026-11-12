/*
 * Behavioral self-test for mayhem/test.sh: known descriptors / requests in, known parsed state / replies out.
 * Prints one "PASS <name>" or "FAIL <name>" line per check.
 */
#include <stdio.h>
#include <string.h>
#include "usbh_core.h"
#include "usb_desc_check.h"
#include "fake_hc.h"
#include "fake_dc.h"

static int g_pass, g_fail;

static void check(const char *name, int ok)
{
    printf("%s %s\n", ok ? "PASS" : "FAIL", name);
    if (ok) {
        g_pass++;
    } else {
        g_fail++;
    }
}

/* USB 2.0 high-speed device 1234:5678 with a single mass-storage (SCSI, bulk-only) interface */
static const uint8_t dev_desc[18] = { 18, 1, 0x00, 0x02, 0, 0, 0, 64, 0x34, 0x12, 0x78, 0x56, 0x00, 0x01, 0, 0, 0, 1 };
static const uint8_t msc_cfg[32] = {
    9, 2, 32, 0, 1, 1, 0, 0x80, 50,
    9, 4, 0, 0, 2, 8, 6, 0x50, 0,
    7, 5, 0x81, 2, 0x00, 0x02, 0,
    7, 5, 0x02, 2, 0x00, 0x02, 0,
};
/* CDC-ACM with an interface association: comm interface 0 (notification EP 0x83) + data interface 1 */
static const uint8_t cdc_cfg[75] = {
    9, 2, 75, 0, 2, 1, 0, 0x80, 50,
    8, 0x0B, 0, 2, 2, 2, 0, 0,
    9, 4, 0, 0, 1, 2, 2, 0, 0,
    5, 0x24, 0, 0x10, 1, 5, 0x24, 1, 0, 1, 4, 0x24, 2, 2, 5, 0x24, 6, 0, 1,
    7, 5, 0x83, 3, 8, 0, 10,
    9, 4, 1, 0, 2, 0x0A, 0, 0, 0,
    7, 5, 0x02, 2, 0x00, 0x02, 0,
    7, 5, 0x81, 2, 0x00, 0x02, 0,
};

static void test_desc_check(void)
{
    uint8_t bad[sizeof(cdc_cfg)];
    uint32_t off = 99;

    check("desc_check accepts MSC config", usb_desc_check_config_desc(USB_SPEED_HIGH, msc_cfg, sizeof(msc_cfg), &off) == 0);
    check("desc_check accepts CDC-ACM config", usb_desc_check_config_desc(USB_SPEED_HIGH, cdc_cfg, sizeof(cdc_cfg), &off) == 0);

    memcpy(bad, cdc_cfg, sizeof(bad));
    bad[9 + 8 + 4] = 5; /* comm interface claims 5 endpoints but has 1 */
    check("desc_check rejects wrong bNumEndpoints", usb_desc_check_config_desc(USB_SPEED_HIGH, bad, sizeof(bad), &off) < 0);

    memcpy(bad, msc_cfg, sizeof(msc_cfg));
    bad[2] = 40; /* wTotalLength larger than the buffer */
    check("desc_check rejects oversized wTotalLength", usb_desc_check_config_desc(USB_SPEED_HIGH, bad, sizeof(msc_cfg), &off) < 0);
}

static void test_host(void)
{
    struct usbh_hubport *hp;
    uint8_t lun0 = 0;
    uint8_t bad_dev[18];
    int ret;

    mayhem_host_register_classes();

    hp = mayhem_host_enumerate(USB_SPEED_HIGH, dev_desc, msc_cfg, sizeof(msc_cfg), &lun0, 1, &ret);
    check("host parses idVendor/idProduct", hp->device_desc.idVendor == 0x1234 && hp->device_desc.idProduct == 0x5678);
    check("host assigns a device address", hp->dev_addr >= 2);
    check("host parses config header", hp->config.config_desc.bNumInterfaces == 1 && hp->config.config_desc.wTotalLength == 32);
    check("host parses MSC interface",
          hp->config.intf[0].altsetting[0].intf_desc.bInterfaceClass == 8 &&
              hp->config.intf[0].altsetting[0].intf_desc.bInterfaceSubClass == 6 &&
              hp->config.intf[0].altsetting[0].intf_desc.bNumEndpoints == 2);
    check("host parses bulk endpoints",
          hp->config.intf[0].altsetting[0].ep[0].ep_desc.bEndpointAddress == 0x81 &&
              hp->config.intf[0].altsetting[0].ep[1].ep_desc.bEndpointAddress == 0x02 &&
              hp->config.intf[0].altsetting[0].ep[1].ep_desc.wMaxPacketSize == 512);
    check("host binds the msc class driver",
          hp->config.intf[0].class_driver != NULL && strcmp(hp->config.intf[0].class_driver->driver_name, "msc") == 0);
    mayhem_host_release(hp);

    hp = mayhem_host_enumerate(USB_SPEED_HIGH, dev_desc, cdc_cfg, sizeof(cdc_cfg), NULL, 0, &ret);
    check("host parses CDC data interface endpoints",
          hp->config.config_desc.bNumInterfaces == 2 &&
              hp->config.intf[1].altsetting[0].intf_desc.bInterfaceClass == 0x0A &&
              hp->config.intf[1].altsetting[0].ep[0].ep_desc.bEndpointAddress == 0x02 &&
              hp->config.intf[1].altsetting[0].ep[1].ep_desc.bEndpointAddress == 0x81);
    check("host binds CDC comm interface to cdc_acm",
          hp->config.intf[0].class_driver != NULL && strcmp(hp->config.intf[0].class_driver->driver_name, "cdc_acm") == 0);
    mayhem_host_release(hp);

    memcpy(bad_dev, dev_desc, sizeof(bad_dev));
    bad_dev[0] = 17; /* wrong bLength */
    hp = mayhem_host_enumerate(USB_SPEED_HIGH, bad_dev, msc_cfg, sizeof(msc_cfg), &lun0, 1, &ret);
    check("host rejects a device descriptor with bad bLength", ret < 0 && hp->device_desc.idVendor == 0);
    mayhem_host_release(hp);

    memcpy(bad_dev, dev_desc, sizeof(bad_dev));
    bad_dev[7] = 24; /* bMaxPacketSize0 invalid for high speed */
    hp = mayhem_host_enumerate(USB_SPEED_HIGH, bad_dev, msc_cfg, sizeof(msc_cfg), &lun0, 1, &ret);
    check("host rejects invalid bMaxPacketSize0 for the speed", ret < 0);
    mayhem_host_release(hp);
}

static void setup(uint8_t type, uint8_t req, uint16_t value, uint16_t index, uint16_t length)
{
    uint8_t s[8] = { type, req, (uint8_t)value, (uint8_t)(value >> 8), (uint8_t)index, (uint8_t)(index >> 8),
                     (uint8_t)length, (uint8_t)(length >> 8) };
    mayhem_device_setup(s);
}

static void test_device(void)
{
    const uint8_t *w;
    uint8_t lc_in[7] = { 0x00, 0xC2, 0x01, 0x00, 0x00, 0x00, 0x08 }; /* 115200 8N1 */
    uint8_t cbw[31] = { 'U', 'S', 'B', 'C', 0x78, 0x56, 0x34, 0x12, 36, 0, 0, 0, 0x80, 0, 6, 0x12, 0, 0, 0, 36, 0 };

    mayhem_device_init();
    mayhem_device_reset();

    setup(0x80, 6, 0x0100, 0, 18);
    w = mayhem_device_written(0);
    check("device answers GET_DESCRIPTOR(device)",
          mayhem_device_pending_write(0) == 18 && w[0] == 18 && w[1] == 1 && w[2] == 0x00 && w[3] == 0x02 && w[8] == 0xFF && w[9] == 0xFF);
    mayhem_device_in_done(0, 18);

    setup(0x80, 6, 0x0200, 0, 9);
    w = mayhem_device_written(0);
    check("device answers GET_DESCRIPTOR(config) header",
          mayhem_device_pending_write(0) == 9 && w[0] == 9 && w[1] == 2 && w[4] == 4 && w[5] == 1);
    mayhem_device_in_done(0, 9);

    setup(0x00, 9, 1, 0, 0);
    mayhem_device_in_done(0, 0);
    setup(0x80, 8, 0, 0, 1);
    w = mayhem_device_written(0);
    check("device reports the selected configuration", mayhem_device_pending_write(0) == 1 && w[0] == 1);
    mayhem_device_in_done(0, 1);

    setup(0x21, 0x20, 0, 0, 7);
    mayhem_device_out(0, lc_in, 7);
    setup(0xA1, 0x21, 0, 0, 7);
    w = mayhem_device_written(0);
    check("cdc line coding round-trips through the control pipe",
          mayhem_device_pending_write(0) == 7 && memcmp(w, lc_in, 7) == 0);
    mayhem_device_in_done(0, 7);

    setup(0xA1, 0xFE, 0, 2, 1);
    w = mayhem_device_written(0);
    check("msc reports max LUN 0", mayhem_device_pending_write(0) == 1 && w[0] == 0);
    mayhem_device_in_done(0, 1);

    mayhem_device_out(5, cbw, sizeof(cbw));
    w = mayhem_device_written(4);
    check("msc answers INQUIRY with a 36 byte standard response",
          mayhem_device_pending_write(4) == 36 && w[0] == 0x00 && w[3] == 0x02 && w[4] == 31);

    setup(0x81, 6, 0x2200, 3, 74);
    w = mayhem_device_written(0);
    check("hid serves the mouse report descriptor", mayhem_device_pending_write(0) == 74 && w[0] == 0x05 && w[1] == 0x01 && w[3] == 0x02);
    mayhem_device_in_done(0, 74);
}

int main(void)
{
    test_desc_check();
    test_host();
    test_device();
    printf("SUMMARY passed=%d failed=%d\n", g_pass, g_fail);
    return g_fail != 0;
}
