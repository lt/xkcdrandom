#include <linux/module.h>
#include <linux/slab.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>

#include <asm/uaccess.h>

#include "xkcdrandom.h"

MODULE_AUTHOR("Leigh T <leight@gmail.com>");
MODULE_LICENSE("GPL");

static dev_t device_num = 0;
static struct cdev *device_cdev = NULL;
static struct class *device_class = NULL;

struct file_operations fops = {
		.owner =    THIS_MODULE,
		.read =     device_read,
		.write =    device_write,
		.open =     device_open,
		.release =  device_close
};

ssize_t device_read(struct file *filp, char __user *buf, size_t count, loff_t *f_pos)
{
	if (*f_pos > 0) {
		return 0;
	}

	if (copy_to_user(buf, "4", 1) != 0) {
		return -EFAULT;
	}

	*f_pos += 1;
	return 1;
}

ssize_t device_write(struct file *filp, const char __user *buf, size_t count, loff_t *f_pos)
{
	return -EPERM;
}

int device_open(struct inode *inode, struct file *filp)
{
	if (inode->i_cdev != device_cdev || inode->i_rdev != device_num) {
		printk(KERN_WARNING "WTF?");
		return -ENODEV;
	}

	return 0;
}

int device_close(struct inode *inode, struct file *filp)
{
	return 0;
}

static void mod_cleanup(void)
{
	if (device_cdev) {
		device_destroy(device_class, device_num);
		cdev_del(device_cdev);
		kfree(device_cdev);
	}
	
	if (device_class) {
		class_destroy(device_class);
	}

	unregister_chrdev_region(device_num, 1);
	return;
}

static int __init mod_init(void)
{
	int errno = 0;
	struct device *device = NULL;

	errno = alloc_chrdev_region(&device_num, 0, 1, DEVICE_NAME);
	if (errno < 0) {
		printk(KERN_WARNING "alloc_chrdev_region failed (%d)", errno);
		return errno;
	}

	device_class = class_create(THIS_MODULE, DEVICE_NAME);
	if (IS_ERR(device_class)) {
		errno = PTR_ERR(device_class);
		printk(KERN_WARNING "class_create failed (%d)", errno);
		goto fail;
	}

	device_cdev = kzalloc(sizeof(struct cdev), GFP_KERNEL);
	if (device_cdev == NULL) {
		printk(KERN_WARNING "kzalloc failed");
		errno = -ENOMEM;
		goto fail;
	}

	cdev_init(device_cdev, &fops);
	device_cdev->owner = THIS_MODULE;

	errno = cdev_add(device_cdev, device_num, 1);
	if (errno) {
		printk(KERN_WARNING "cdev_add failed (%d)", errno);
		goto fail;
	}

	device = device_create(device_class, NULL, device_num, NULL, DEVICE_NAME);
	if (IS_ERR(device)) {
		errno = PTR_ERR(device);
		printk(KERN_WARNING "device_create failed (%d)", errno);
		cdev_del(device_cdev);
		goto fail;
	}

	return 0;

fail:
	mod_cleanup();
	return errno;
}

static void __exit mod_exit(void)
{
	mod_cleanup();
	return;
}

module_init(mod_init);
module_exit(mod_exit);