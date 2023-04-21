#include "device.h"
#include "tty.h"
#include "uart.h"
#include <platform/platform.h>
#include <uniks/kassert.h>
#include <uniks/kstring.h>
#include <uniks/log.h>


/**
 * @brief device array, and devices[0] will never be used. Moreover, the actual
 * device is corresponding to the PLIC intterupt number in platform.h. Such as
 * when the platform is QEMU, and UART0_IRQ = 10, prescribed by QEMU,
 * devices[10] is UART0 device! etc.
 */
struct device_t devices[NDEVICE];

void device_init()
{
	for (int32_t i = 1; i < NDEVICE; i++) {
		struct device_t *device = &devices[i];
		device->type = device->subtype = DEV_NULL;
		device->dev_id = i;
		device->parent = 0;
		device->interrupt_handler = NULL;
		device->ioctl = NULL;
		device->read = device->write = NULL;
	}

	// install tty device internal
	tty_init();
	// then install uart device internal
	uart_init();
}

/**
 * @brief Get the null virtual device
 * @return struct device_t*
 */
struct device_t *get_null_device()
{
	struct device_t *dev;
	int32_t i = MAX_INTTERUPT_SRC_ID + 1;
	while (i < NDEVICE) {
		dev = &devices[i++];
		if (dev->type == DEV_NULL)
			return dev;
	}
	return NULL;
}

/**
 * @brief install a device. And we assume that this will be called only
 * when kernel boot, so there are no need to acquire a lock.
 * @param type
 * @param subtype
 * @param ptr
 * @param name
 * @param parent
 * @param interrupt_handler
 * @param ioctl
 * @param read
 * @param write
 * @param device
 * @return dev_t
 */
dev_t device_install(int32_t type, int32_t subtype, void *ptr, char *name,
		     dev_t parent, void *interrupt_handler, void *ioctl,
		     void *read, void *write, struct device_t *device)
{
	device->ptr = ptr;
	device->parent = parent;
	device->type = type;
	device->subtype = subtype;
	device->interrupt_handler = interrupt_handler;
	device->ioctl = ioctl;
	device->read = read;
	device->write = write;
	strncpy(device->name, name, DEV_NAME_LEN);
	return device->dev_id;
}

/**
 * @brief Unified process I/O device interrupt request generated by PLIC. This
 * function is called in plic.c:external_interrupt_handler()
 * @param dev
 */
void device_interrupt_handler(dev_t dev)
{
	assert(devices[dev].type != DEV_NULL);
	devices[dev].interrupt_handler(devices[dev].ptr);
	// force cause parent device interrupt recursively
	dev_t dev_parent = devices[dev].parent;
	if (dev_parent)
		device_interrupt_handler(dev_parent);
}