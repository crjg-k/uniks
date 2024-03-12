#ifndef __KERNEL_DEVICE_BLK_DEV_H__
#define __KERNEL_DEVICE_BLK_DEV_H__


#include <uniks/defs.h>


int64_t blkdev_read(dev_t dev, char *buf, int64_t pos, size_t cnt);
int64_t blkdev_write(dev_t dev, char *buf, int64_t pos, size_t cnt);


#endif /* !__KERNEL_DEVICE_BLK_DEV_H__ */