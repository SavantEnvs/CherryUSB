/* Fake device controller + a CDC-ACM/MSC/HID composite device (descriptors adapted from demo/cdc_acm_hid_msc_template.c). */
#include <string.h>
#include "usbd_core.h"
#include "usbd_msc.h"
#include "usbd_cdc_acm.h"
#include "usbd_hid.h"
#include "fake_dc.h"

/*!< endpoint address */
#define CDC_IN_EP  0x81
#define CDC_OUT_EP 0x02
#define CDC_INT_EP 0x83

#define MSC_IN_EP  0x84
#define MSC_OUT_EP 0x05

/*!< endpoint address */
#define HID_INT_EP          0x86
#define HID_INT_EP_SIZE     4
#define HID_INT_EP_INTERVAL 1

#define USBD_VID           0xFFFF
#define USBD_PID           0xFFFF
#define USBD_MAX_POWER     100
#define USBD_LANGID_STRING 1033

/*!< report descriptor size */
#define HID_MOUSE_REPORT_DESC_SIZE 74

#define USB_CONFIG_SIZE (9 + CDC_ACM_DESCRIPTOR_LEN + MSC_DESCRIPTOR_LEN + 25)

static const uint8_t device_descriptor[] = {
    USB_DEVICE_DESCRIPTOR_INIT(USB_2_0, 0x00, 0x00, 0x00, USBD_VID, USBD_PID, 0x0200, 0x01)
};

static const uint8_t config_descriptor_hs[] = {
    USB_CONFIG_DESCRIPTOR_INIT(USB_CONFIG_SIZE, 0x04, 0x01, USB_CONFIG_BUS_POWERED, USBD_MAX_POWER),
    CDC_ACM_DESCRIPTOR_INIT(0x00, CDC_INT_EP, CDC_OUT_EP, CDC_IN_EP, USB_BULK_EP_MPS_HS, 0x02),
    MSC_DESCRIPTOR_INIT(0x02, MSC_OUT_EP, MSC_IN_EP, USB_BULK_EP_MPS_HS, 0x02),
    HID_MOUSE_DESCRIPTOR_INIT(0x03, 0x01, HID_MOUSE_REPORT_DESC_SIZE, HID_INT_EP, HID_INT_EP_SIZE, HID_INT_EP_INTERVAL),
};

static const uint8_t config_descriptor_fs[] = {
    USB_CONFIG_DESCRIPTOR_INIT(USB_CONFIG_SIZE, 0x04, 0x01, USB_CONFIG_BUS_POWERED, USBD_MAX_POWER),
    CDC_ACM_DESCRIPTOR_INIT(0x00, CDC_INT_EP, CDC_OUT_EP, CDC_IN_EP, USB_BULK_EP_MPS_FS, 0x02),
    MSC_DESCRIPTOR_INIT(0x02, MSC_OUT_EP, MSC_IN_EP, USB_BULK_EP_MPS_FS, 0x02),
    HID_MOUSE_DESCRIPTOR_INIT(0x03, 0x01, HID_MOUSE_REPORT_DESC_SIZE, HID_INT_EP, HID_INT_EP_SIZE, HID_INT_EP_INTERVAL),
};

static const uint8_t device_quality_descriptor[] = {
    USB_DEVICE_QUALIFIER_DESCRIPTOR_INIT(USB_2_0, 0x00, 0x00, 0x00, 0x01),
};

static const uint8_t other_speed_config_descriptor_hs[] = {
    USB_OTHER_SPEED_CONFIG_DESCRIPTOR_INIT(USB_CONFIG_SIZE, 0x04, 0x01, USB_CONFIG_BUS_POWERED, USBD_MAX_POWER),
    CDC_ACM_DESCRIPTOR_INIT(0x00, CDC_INT_EP, CDC_OUT_EP, CDC_IN_EP, USB_BULK_EP_MPS_FS, 0x02),
    MSC_DESCRIPTOR_INIT(0x02, MSC_OUT_EP, MSC_IN_EP, USB_BULK_EP_MPS_FS, 0x02),
    HID_MOUSE_DESCRIPTOR_INIT(0x03, 0x01, HID_MOUSE_REPORT_DESC_SIZE, HID_INT_EP, HID_INT_EP_SIZE, HID_INT_EP_INTERVAL),
};

static const uint8_t other_speed_config_descriptor_fs[] = {
    USB_OTHER_SPEED_CONFIG_DESCRIPTOR_INIT(USB_CONFIG_SIZE, 0x04, 0x01, USB_CONFIG_BUS_POWERED, USBD_MAX_POWER),
    CDC_ACM_DESCRIPTOR_INIT(0x00, CDC_INT_EP, CDC_OUT_EP, CDC_IN_EP, USB_BULK_EP_MPS_HS, 0x02),
    MSC_DESCRIPTOR_INIT(0x02, MSC_OUT_EP, MSC_IN_EP, USB_BULK_EP_MPS_HS, 0x02),
    HID_MOUSE_DESCRIPTOR_INIT(0x03, 0x01, HID_MOUSE_REPORT_DESC_SIZE, HID_INT_EP, HID_INT_EP_SIZE, HID_INT_EP_INTERVAL),
};

static const char *string_descriptors[] = {
    (const char[]){ 0x09, 0x04 }, /* Langid */
    "CherryUSB",                  /* Manufacturer */
    "CherryUSB CDC MSC HID DEMO", /* Product */
    "2022123456",                 /* Serial Number */
};

static const uint8_t *device_descriptor_callback(uint8_t speed)
{
    (void)speed;

    return device_descriptor;
}

static const uint8_t *config_descriptor_callback(uint8_t speed)
{
    /* the core only learns the bus speed at the first GET_DESCRIPTOR(device), so answer for any speed */
    return speed == USB_SPEED_FULL ? config_descriptor_fs : config_descriptor_hs;
}

static const uint8_t *device_quality_descriptor_callback(uint8_t speed)
{
    (void)speed;

    return device_quality_descriptor;
}

static const uint8_t *other_speed_config_descriptor_callback(uint8_t speed)
{
    return speed == USB_SPEED_FULL ? other_speed_config_descriptor_fs : other_speed_config_descriptor_hs;
}

static const char *string_descriptor_callback(uint8_t speed, uint8_t index)
{
    (void)speed;

    if (index >= (sizeof(string_descriptors) / sizeof(char *))) {
        return NULL;
    }
    return string_descriptors[index];
}

const struct usb_descriptor cdc_acm_hid_msc_descriptor = {
    .device_descriptor_callback = device_descriptor_callback,
    .config_descriptor_callback = config_descriptor_callback,
    .device_quality_descriptor_callback = device_quality_descriptor_callback,
    .other_speed_descriptor_callback = other_speed_config_descriptor_callback,
    .string_descriptor_callback = string_descriptor_callback
};

/*!< hid mouse report descriptor */
static const uint8_t hid_mouse_report_desc[HID_MOUSE_REPORT_DESC_SIZE] = {
    0x05, 0x01, // USAGE_PAGE (Generic Desktop)
    0x09, 0x02, // USAGE (Mouse)
    0xA1, 0x01, // COLLECTION (Application)
    0x09, 0x01, // USAGE (Pointer)
    0xA1, 0x00, // COLLECTION (Physical)
    0x05, 0x09, // USAGE_PAGE (Button)
    0x19, 0x01, // USAGE_MINIMUM (Button 1)
    0x29, 0x03, // USAGE_MAXIMUM (Button 3)
    0x15, 0x00, // LOGICAL_MINIMUM (0)
    0x25, 0x01, // LOGICAL_MAXIMUM (1)
    0x95, 0x03, // REPORT_COUNT (3)
    0x75, 0x01, // REPORT_SIZE (1)
    0x81, 0x02, // INPUT (Data,Var,Abs)
    0x95, 0x01, // REPORT_COUNT (1)
    0x75, 0x05, // REPORT_SIZE (5)
    0x81, 0x01, // INPUT (Cnst,Var,Abs)
    0x05, 0x01, // USAGE_PAGE (Generic Desktop)
    0x09, 0x30, // USAGE (X)
    0x09, 0x31, // USAGE (Y)
    0x09, 0x38, // Usage (Wheel)
    0x15, 0x81, // LOGICAL_MINIMUM (-127)
    0x25, 0x7F, // LOGICAL_MAXIMUM (127)
    0x75, 0x08, // REPORT_SIZE (8)
    0x95, 0x03, // REPORT_COUNT (2)
    0x81, 0x06, // INPUT (Data,Var,Rel)
    0xC0,       // END_COLLECTION
    0x09, 0x3c, // USAGE (Motion Wakeup)
    0x05, 0xff, // USAGE_PAGE (Vendor Defined 0xFF)
    0x09, 0x01, // USAGE (Vendor Usage 1)
    0x15, 0x00, // LOGICAL_MINIMUM (0)
    0x25, 0x01, // LOGICAL_MAXIMUM (1)
    0x75, 0x01, // REPORT_SIZE (1)
    0x95, 0x02, // REPORT_COUNT (2)
    0xb1, 0x22, // FEATURE (Data,Var,Abs,NPrf)
    0x75, 0x06, // REPORT_SIZE (6)
    0x95, 0x01, // REPORT_COUNT (1)
    0xb1, 0x01, // FEATURE (Cnst,Ary,Abs)
    0xc0        // END_COLLECTION
};


#define RAM_SECTORS 16
#define RAM_SECTOR_SIZE 512
static uint8_t g_ramdisk[RAM_SECTORS * RAM_SECTOR_SIZE];

static struct {
    uint8_t *buf;
    uint32_t len;
} g_rd[16];
static uint32_t g_wr_len[16];
static uint8_t g_wr_copy[16][1024];
static struct cdc_line_coding g_line_coding;
static uint8_t g_stalled[32];

void usbd_msc_get_cap(uint8_t busid, uint8_t lun, uint32_t *block_num, uint32_t *block_size)
{
    (void)busid; (void)lun;
    *block_num = RAM_SECTORS;
    *block_size = RAM_SECTOR_SIZE;
}

int usbd_msc_sector_read(uint8_t busid, uint8_t lun, uint32_t sector, uint8_t *buffer, uint32_t length)
{
    (void)busid; (void)lun;
    if (sector >= RAM_SECTORS || length > (RAM_SECTORS - sector) * RAM_SECTOR_SIZE) {
        return -1;
    }
    memcpy(buffer, g_ramdisk + sector * RAM_SECTOR_SIZE, length);
    return 0;
}

int usbd_msc_sector_write(uint8_t busid, uint8_t lun, uint32_t sector, uint8_t *buffer, uint32_t length)
{
    (void)busid; (void)lun;
    if (sector >= RAM_SECTORS || length > (RAM_SECTORS - sector) * RAM_SECTOR_SIZE) {
        return -1;
    }
    memcpy(g_ramdisk + sector * RAM_SECTOR_SIZE, buffer, length);
    return 0;
}

int usb_dc_init(uint8_t busid) { (void)busid; return 0; }
int usb_dc_deinit(uint8_t busid) { (void)busid; return 0; }
int usbd_set_address(uint8_t busid, const uint8_t addr) { (void)busid; (void)addr; return 0; }
int usbd_set_remote_wakeup(uint8_t busid) { (void)busid; return 0; }
uint8_t usbd_get_port_speed(uint8_t busid) { (void)busid; return USB_SPEED_HIGH; }
int usbd_ep_open(uint8_t busid, const struct usb_endpoint_descriptor *ep) { (void)busid; (void)ep; return 0; }
int usbd_ep_close(uint8_t busid, const uint8_t ep) { (void)busid; (void)ep; return 0; }

int usbd_ep_set_stall(uint8_t busid, const uint8_t ep)
{
    (void)busid;
    g_stalled[ep & 0x1f] = 1;
    if (ep & 0x80) {
        g_wr_len[ep & 0xf] = 0; /* a stall flushes the IN FIFO: no completion can follow */
    }
    return 0;
}

int usbd_ep_clear_stall(uint8_t busid, const uint8_t ep)
{
    (void)busid;
    g_stalled[ep & 0x1f] = 0;
    return 0;
}

int usbd_ep_is_stalled(uint8_t busid, const uint8_t ep, uint8_t *stalled)
{
    (void)busid;
    *stalled = g_stalled[ep & 0x1f];
    return 0;
}

int usbd_ep_start_write(uint8_t busid, const uint8_t ep, const uint8_t *data, uint32_t data_len)
{
    (void)busid;
    g_wr_len[ep & 0xf] = data_len;
    if (data != NULL) {
        memcpy(g_wr_copy[ep & 0xf], data, data_len < sizeof(g_wr_copy[0]) ? data_len : sizeof(g_wr_copy[0]));
    }
    return 0;
}

int usbd_ep_start_read(uint8_t busid, const uint8_t ep, uint8_t *data, uint32_t data_len)
{
    (void)busid;
    g_rd[ep & 0xf].buf = data;
    g_rd[ep & 0xf].len = data_len;
    return 0;
}

void usbd_cdc_acm_set_line_coding(uint8_t busid, uint8_t intf, struct cdc_line_coding *line_coding)
{
    (void)busid; (void)intf;
    g_line_coding = *line_coding;
}

void usbd_cdc_acm_get_line_coding(uint8_t busid, uint8_t intf, struct cdc_line_coding *line_coding)
{
    (void)busid; (void)intf;
    *line_coding = g_line_coding;
}

static void device_event(uint8_t busid, uint8_t event)
{
    (void)busid; (void)event;
}

static struct usbd_interface intf0;
static struct usbd_interface intf1;
static struct usbd_interface intf2;
static struct usbd_interface intf3;

static void hid_int_cb(uint8_t busid, uint8_t ep, uint32_t nbytes)
{
    (void)busid; (void)ep; (void)nbytes;
}

static uint8_t g_cdc_rx[2048];

static void cdc_bulk_out(uint8_t busid, uint8_t ep, uint32_t nbytes)
{
    (void)ep; (void)nbytes;
    usbd_ep_start_read(busid, CDC_OUT_EP, g_cdc_rx, sizeof(g_cdc_rx));
}

static void cdc_bulk_in(uint8_t busid, uint8_t ep, uint32_t nbytes)
{
    if ((nbytes % usbd_get_ep_mps(busid, ep)) == 0 && nbytes) {
        usbd_ep_start_write(busid, CDC_IN_EP, NULL, 0);
    }
}

static struct usbd_endpoint cdc_out_ep = { .ep_addr = CDC_OUT_EP, .ep_cb = cdc_bulk_out };
static struct usbd_endpoint cdc_in_ep = { .ep_addr = CDC_IN_EP, .ep_cb = cdc_bulk_in };
static struct usbd_endpoint hid_in_ep = { .ep_addr = HID_INT_EP, .ep_cb = hid_int_cb };

/* Registers the composite device from scratch: usbd_desc_register() zeroes g_usbd_core[0] and
 * usbd_msc_init_intf() zeroes g_usbd_msc[0], so no device-stack state survives from the previous input. */
void mayhem_device_init(void)
{
    memset(&intf0, 0, sizeof(intf0));
    memset(&intf1, 0, sizeof(intf1));
    memset(&intf2, 0, sizeof(intf2));
    memset(&intf3, 0, sizeof(intf3));
    usbd_desc_register(0, &cdc_acm_hid_msc_descriptor);
    usbd_add_interface(0, usbd_cdc_acm_init_intf(0, &intf0));
    usbd_add_interface(0, usbd_cdc_acm_init_intf(0, &intf1));
    usbd_add_endpoint(0, &cdc_out_ep);
    usbd_add_endpoint(0, &cdc_in_ep);
    usbd_add_interface(0, usbd_msc_init_intf(0, &intf2, MSC_OUT_EP, MSC_IN_EP));
    usbd_add_interface(0, usbd_hid_init_intf(0, &intf3, hid_mouse_report_desc, HID_MOUSE_REPORT_DESC_SIZE));
    usbd_add_endpoint(0, &hid_in_ep);
    usbd_initialize(0, 0, device_event);
}

void mayhem_device_reset(void)
{
    memset(g_ramdisk, 0, sizeof(g_ramdisk));
    memset(g_rd, 0, sizeof(g_rd));
    memset(g_wr_len, 0, sizeof(g_wr_len));
    memset(g_wr_copy, 0, sizeof(g_wr_copy));
    memset(&g_line_coding, 0, sizeof(g_line_coding));
    memset(g_stalled, 0, sizeof(g_stalled));
    memset(g_cdc_rx, 0, sizeof(g_cdc_rx));
    usbd_event_reset_handler(0);
}

void mayhem_device_setup(const uint8_t setup[8])
{
    uint8_t buf[8];
    memcpy(buf, setup, 8);
    g_wr_len[0] = 0; /* a SETUP flushes the EP0 IN FIFO: no completion for the old transfer */
    usbd_event_ep0_setup_complete_handler(0, buf);
}

void mayhem_device_out(uint8_t ep, const uint8_t *data, uint32_t len)
{
    uint32_t n;
    if (g_rd[ep & 0xf].buf == NULL) {
        return;
    }
    n = len < g_rd[ep & 0xf].len ? len : g_rd[ep & 0xf].len;
    memcpy(g_rd[ep & 0xf].buf, data, n);
    usbd_event_ep_out_complete_handler(0, ep & 0xf, n);
}

void mayhem_device_in_done(uint8_t ep, uint32_t nbytes)
{
    uint32_t pending = g_wr_len[ep & 0xf];
    if (pending == 0) {
        return;
    }
    g_wr_len[ep & 0xf] = 0;
    usbd_event_ep_in_complete_handler(0, 0x80 | (ep & 0xf), nbytes < pending ? nbytes : pending);
}

uint32_t mayhem_device_pending_write(uint8_t ep)
{
    return g_wr_len[ep & 0xf];
}

const uint8_t *mayhem_device_written(uint8_t ep)
{
    return g_wr_copy[ep & 0xf];
}
