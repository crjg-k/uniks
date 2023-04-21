#ifndef __KERNEL_DEVICE_DEVICE_H__
#define __KERNEL_DEVICE_DEVICE_H__

#include <uniks/defs.h>
#include <uniks/param.h>


// fundamental device type
enum device_type_t {
	DEV_NULL,    // null device
	DEV_CHAR,    // character device
	DEV_BLOCK,   // block device
};

// device sub-type
enum device_subtype_t {
	DEV_TTY,		// tty
	DEV_SERIAL,		// UART
	DEV_EXTERNAL_STORAGE,	// secondary storage
	DEV_RAM_VIRT_STORAGE,	// vitual secondary storage
};

struct device_t {
	enum device_type_t type;	 // device type
	enum device_subtype_t subtype;	 // device subtype
	dev_t dev_id;			 // device id
	dev_t parent;			 // device parent number
	char name[DEV_NAME_LEN];	 // device name
	void *ptr;   // pointer to distinguish which, when exists multi-devs

	// device interrupt handler
	void (*interrupt_handler)(void *ptr);
	// device controlling
	int32_t (*ioctl)(void *ptr);
	// device reading
	int32_t (*read)(void *ptr, void *buf, size_t cnt);
	// device writing
	int32_t (*write)(void *ptr, void *buf, size_t cnt);
};

extern struct device_t devices[];

void device_init();
struct device_t *get_null_device();
dev_t device_install(int32_t type, int32_t subtype, void *ptr, char *name,
		     dev_t parent, void *interrupt_handler, void *ioctl,
		     void *read, void *write, struct device_t *device);
void device_interrupt_handler(dev_t dev);


#endif /* !__KERNEL_DEVICE_DEVICE_H__ */