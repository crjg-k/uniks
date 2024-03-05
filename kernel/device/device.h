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
};

struct device_t {
	enum device_type_t type;	 // device type
	enum device_subtype_t subtype;	 // device subtype
	dev_t dev;			 // device id
	dev_t parent;			 // device parent number
	char name[DEV_NAME_LEN];	 // device name
	void *ptr;   // pointer to distinguish which, when exists multi-devs

	// device interrupt handler
	void (*interrupt_handler)(void *ptr);
	// device controlling
	int64_t (*ioctl)(void *ptr);
	// device reading
	int64_t (*read)(void *ptr, int32_t user_dst, void *, size_t);
	// device writing
	int64_t (*write)(void *ptr, int32_t user_src, void *, size_t);
};

extern struct device_t devices[];

void device_init();
struct device_t *get_null_device();
dev_t device_install(int32_t type, int32_t subtype, void *ptr, char *name,
		     dev_t parent, void *interrupt_handler, void *ioctl,
		     void *read, void *write, struct device_t *device);
void device_interrupt_handler(dev_t dev);
int64_t device_read(dev_t dev, int32_t user_dst, void *buf, size_t cnt);
int64_t device_write(dev_t dev, int32_t user_src, void *buf, size_t cnt);


#endif /* !__KERNEL_DEVICE_DEVICE_H__ */