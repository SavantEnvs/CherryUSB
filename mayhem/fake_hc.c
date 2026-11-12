/* Fake host controller: answers every URB from caller-supplied descriptor and payload bytes. */
#include <stddef.h>
#include <stdint.h>
#include "usbh_core.h"
#include "fake_hc.h"

extern const struct usbh_class_info msc_class_info;
extern const struct usbh_class_info hid_custom_class_info;
extern const struct usbh_class_info cdc_acm_none_class_info;
extern const struct usbh_class_info cdc_acm_at_class_info;
extern const struct usbh_class_info cdc_data_class_info;
extern const struct usbh_class_info hub_class_info;

/* usbh_initialize() is never called, but it references the linker-provided class table bounds. */
uint32_t __usbh_class_info_start__;
uint32_t __usbh_class_info_end__;
extern struct usbh_class_info *usbh_class_info_table_begin;
extern struct usbh_class_info *usbh_class_info_table_end;

static struct usbh_class_info g_class_table[6];
static struct usbh_hub g_hub;

static const uint8_t *g_dev;
static const uint8_t *g_cfg;
static uint32_t g_cfg_len;
static const uint8_t *g_extra;
static uint32_t g_extra_len;
static uint32_t g_extra_pos;

static uint32_t serve(uint8_t *dst, uint32_t want, const uint8_t *src, uint32_t avail)
{
    uint32_t n = want < avail ? want : avail;
    for (uint32_t i = 0; i < n; i++) {
        dst[i] = src[i];
    }
    return n;
}

static uint32_t serve_extra(uint8_t *dst, uint32_t want)
{
    uint32_t n = serve(dst, want, g_extra + g_extra_pos, g_extra_len - g_extra_pos);
    g_extra_pos += n;
    return n;
}

int usbh_submit_urb(struct usbh_urb *urb)
{
    uint32_t n;

    if (urb == NULL || urb->hport == NULL) {
        return -USB_ERR_INVAL;
    }

    if (urb->setup != NULL) {
        struct usb_setup_packet *s = urb->setup;
        uint32_t want = s->wLength;

        if (!(s->bmRequestType & USB_REQUEST_DIR_IN) || want == 0 || urb->transfer_buffer == NULL) {
            urb->actual_length = sizeof(struct usb_setup_packet) + (s->bmRequestType & USB_REQUEST_DIR_IN ? 0 : want);
            return 0;
        }
        if (want > urb->transfer_buffer_length) {
            want = urb->transfer_buffer_length;
        }
        if ((s->bmRequestType & USB_REQUEST_TYPE_MASK) == USB_REQUEST_STANDARD &&
            s->bRequest == USB_REQUEST_GET_DESCRIPTOR && (s->wValue >> 8) == USB_DESCRIPTOR_TYPE_DEVICE) {
            n = serve(urb->transfer_buffer, want, g_dev, USB_SIZEOF_DEVICE_DESC);
        } else if ((s->bmRequestType & USB_REQUEST_TYPE_MASK) == USB_REQUEST_STANDARD &&
                   s->bRequest == USB_REQUEST_GET_DESCRIPTOR && (s->wValue >> 8) == USB_DESCRIPTOR_TYPE_CONFIGURATION) {
            n = serve(urb->transfer_buffer, want, g_cfg, g_cfg_len);
        } else {
            n = serve_extra(urb->transfer_buffer, want);
        }
        urb->actual_length = sizeof(struct usb_setup_packet) + n;
        return 0;
    }

    if (urb->ep != NULL && (urb->ep->bEndpointAddress & 0x80) && urb->transfer_buffer != NULL) {
        n = serve_extra(urb->transfer_buffer, urb->transfer_buffer_length);
        if (n == 0) {
            return -USB_ERR_IO;
        }
        urb->actual_length = n;
        return 0;
    }
    urb->actual_length = urb->transfer_buffer_length;
    return 0;
}

int usbh_kill_urb(struct usbh_urb *urb)
{
    (void)urb;
    return 0;
}

int usb_hc_init(struct usbh_bus *bus)
{
    (void)bus;
    return 0;
}

int usb_hc_deinit(struct usbh_bus *bus)
{
    (void)bus;
    return 0;
}

uint16_t usbh_get_frame_number(struct usbh_bus *bus)
{
    (void)bus;
    return 0;
}

int usbh_roothub_control(struct usbh_bus *bus, struct usb_setup_packet *setup, uint8_t *buf)
{
    (void)bus; (void)setup; (void)buf;
    return -USB_ERR_INVAL;
}

static void quiet_event(uint8_t busid, uint8_t hub_index, uint8_t hub_port, uint8_t intf, uint8_t event)
{
    (void)busid; (void)hub_index; (void)hub_port; (void)intf; (void)event;
}

void mayhem_host_register_classes(void)
{
    g_class_table[0] = msc_class_info;
    g_class_table[1] = hid_custom_class_info;
    g_class_table[2] = cdc_acm_none_class_info;
    g_class_table[3] = cdc_acm_at_class_info;
    g_class_table[4] = cdc_data_class_info;
    g_class_table[5] = hub_class_info;
    usbh_class_info_table_begin = &g_class_table[0];
    usbh_class_info_table_end = &g_class_table[6];
}

struct usbh_hubport *mayhem_host_enumerate(uint8_t speed, const uint8_t *dev, const uint8_t *cfg, uint32_t cfg_len,
                                           const uint8_t *extra, uint32_t extra_len, int *ret)
{
    struct usbh_bus *bus = &g_usbhost_bus[0];
    struct usbh_hubport *hport;

    g_dev = dev;
    g_cfg = cfg;
    g_cfg_len = cfg_len;
    g_extra = extra;
    g_extra_len = extra_len;
    g_extra_pos = 0;

    memset(bus, 0, sizeof(*bus));
    bus->busid = 0;
    bus->devgen.last = 0x7f;
    bus->event_handler = quiet_event;

    memset(&g_hub, 0, sizeof(g_hub));
    g_hub.index = 1;
    g_hub.bus = bus;
    g_hub.nports = 1;
    hport = &g_hub.child[0];
    hport->port = 1;
    hport->bus = bus;
    hport->parent = &g_hub;
    hport->speed = speed;
    hport->connected = true;
    hport->mutex = usb_osal_mutex_create();

    *ret = usbh_enumerate(hport);
    return hport;
}

void mayhem_host_release(struct usbh_hubport *hport)
{
    usbh_hubport_release(hport);
}
