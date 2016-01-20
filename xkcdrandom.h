#define DEVICE_NAME "xkcdrandom"

ssize_t device_read(struct file *filp, char __user *buf, size_t count, loff_t *f_pos);
ssize_t device_write(struct file *filp, const char __user *buf, size_t count, loff_t *f_pos);
int device_open(struct inode *inode, struct file *filp);
int device_close(struct inode *inode, struct file *filp);