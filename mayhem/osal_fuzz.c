/* Single-threaded usb_osal for the fuzz harnesses: nothing blocks, nothing is scheduled. */
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include "usb_config.h"
#include "usb_osal.h"

char mayhem_logbuf[256];

usb_osal_thread_t usb_osal_thread_create(const char *name, uint32_t stack_size, uint32_t prio, usb_thread_entry_t entry, void *args)
{
    (void)name; (void)stack_size; (void)prio; (void)entry; (void)args;
    return (usb_osal_thread_t)malloc(1);
}

void usb_osal_thread_delete(usb_osal_thread_t thread)
{
    free(thread);
}

void usb_osal_thread_schedule_other(void)
{
}

usb_osal_sem_t usb_osal_sem_create(uint32_t initial_count)
{
    uint32_t *c = (uint32_t *)malloc(sizeof(*c));
    *c = initial_count;
    return c;
}

usb_osal_sem_t usb_osal_sem_create_counting(uint32_t max_count)
{
    (void)max_count;
    return usb_osal_sem_create(0);
}

void usb_osal_sem_delete(usb_osal_sem_t sem)
{
    free(sem);
}

int usb_osal_sem_take(usb_osal_sem_t sem, uint32_t timeout)
{
    uint32_t *c = (uint32_t *)sem;
    (void)timeout;
    if (c && *c) {
        (*c)--;
        return 0;
    }
    return -1;
}

int usb_osal_sem_give(usb_osal_sem_t sem)
{
    if (sem) {
        (*(uint32_t *)sem)++;
    }
    return 0;
}

void usb_osal_sem_reset(usb_osal_sem_t sem)
{
    if (sem) {
        *(uint32_t *)sem = 0;
    }
}

usb_osal_mutex_t usb_osal_mutex_create(void)
{
    return malloc(1);
}

void usb_osal_mutex_delete(usb_osal_mutex_t mutex)
{
    free(mutex);
}

int usb_osal_mutex_take(usb_osal_mutex_t mutex)
{
    (void)mutex;
    return 0;
}

int usb_osal_mutex_give(usb_osal_mutex_t mutex)
{
    (void)mutex;
    return 0;
}

usb_osal_mq_t usb_osal_mq_create(uint32_t max_msgs)
{
    (void)max_msgs;
    return malloc(1);
}

void usb_osal_mq_delete(usb_osal_mq_t mq)
{
    free(mq);
}

int usb_osal_mq_send(usb_osal_mq_t mq, uintptr_t addr)
{
    (void)mq; (void)addr;
    return 0;
}

int usb_osal_mq_recv(usb_osal_mq_t mq, uintptr_t *addr, uint32_t timeout)
{
    (void)mq; (void)addr; (void)timeout;
    return -1;
}

struct usb_osal_timer *usb_osal_timer_create(const char *name, uint32_t timeout_ms, usb_timer_handler_t handler, void *argument, bool is_period)
{
    struct usb_osal_timer *t = (struct usb_osal_timer *)calloc(1, sizeof(*t));
    (void)name; (void)timeout_ms; (void)handler; (void)argument; (void)is_period;
    return t;
}

void usb_osal_timer_delete(struct usb_osal_timer *timer)
{
    free(timer);
}

void usb_osal_timer_start(struct usb_osal_timer *timer)
{
    (void)timer;
}

void usb_osal_timer_stop(struct usb_osal_timer *timer)
{
    (void)timer;
}

size_t usb_osal_enter_critical_section(void)
{
    return 0;
}

void usb_osal_leave_critical_section(size_t flag)
{
    (void)flag;
}

void usb_osal_msleep(uint32_t delay)
{
    (void)delay;
}

void *usb_osal_malloc(size_t size)
{
    return malloc(size);
}

void usb_osal_free(void *ptr)
{
    free(ptr);
}
