#include <linux/module.h>
#include <linux/moduleparam.h>
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

static bool spam = 0;
static char value = 52; // "4"

module_param(spam, bool, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
MODULE_PARM_DESC(spam, "Don't stop");
module_param(value, byte, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
MODULE_PARM_DESC(value, "What to output");

struct file_operations fops = {
	.owner =    THIS_MODULE,
	.read =     device_read
};

static int device_uevent(struct device *dev, struct kobj_uevent_env *env)
{
	add_uevent_var(env, "DEVMODE=%#o", 0444);
	return 0;
}

ssize_t device_read(struct file *filp, char __user *buf, size_t count, loff_t *f_pos)
{
	if (spam == 0 && *f_pos > 0) {
		return 0;
	}

	if (copy_to_user(buf, &value, 1) != 0) {
		return -EFAULT;
	}

	*f_pos += 1;
	return 1;
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
	device_class->dev_uevent = device_uevent;

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
