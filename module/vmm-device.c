/*****************************************************************************
 * Copyright  2024  Emisilve86                                               *
 *                                                                           *
 * This file is part of VM-Monitor.                                          *
 *                                                                           *
 * VM-Monitor is free software: you can redistribute it and/or modify        *
 * it under the terms of the GNU General Public License as published by      *
 * the Free Software Foundation, either version 3 of the License, or         *
 * (at your option) any later version.                                       *
 *                                                                           *
 * VM-Monitor is distributed in the hope that it will be useful,             *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of            *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the             *
 * GNU General Public License for more details.                              *
 *                                                                           *
 * You should have received a copy of the GNU General Public License         *
 * along with Nome-Programma.  If not, see <http://www.gnu.org/licenses/>.   *
 *****************************************************************************/

#include <linux/err.h>
#include <linux/delay.h>
#include <linux/printk.h>

#include <linux/fs.h>
#include <linux/kdev_t.h>
#include <linux/device.h>

#include "vmm-device.h"


/********************************
 *     STATIC PROTOTYPES        *
 ********************************/

static ssize_t vmm_read(struct file *, char __user *, size_t, loff_t *);
static long vmm_ioctl(struct file *, unsigned int, unsigned long);


/********************************
 *       STATIC VARIABLES       *
 ********************************/

static struct file_operations vmm_dev_fops =
{
	.owner			= THIS_MODULE,
	.read			= vmm_read,
	.unlocked_ioctl	= vmm_ioctl,
};

static int vmm_dev_major;
static struct class *vmm_dev_class;


/********************************
 *       GLOBAL VARIABLES       *
 ********************************/

atomic_t vmm_pid = ATOMIC_INIT(-1);
vmm_walker_counter vw_cnt = { 0 };


/********************************
 *       STATIC FUNCTIONS       *
 ********************************/

static char *vmm_devnode(struct device *dev, umode_t *mode)
{
	return kasprintf(GFP_KERNEL, "vm-monitor");
}

static int vmm_uevent(struct device *dev, struct kobj_uevent_env *env)
{
	add_uevent_var(env, "DEVMODE=%#o", 0666);
	return 0;
}

static ssize_t vmm_read(struct file *filp, char __user *buf, size_t count, loff_t *offset)
{
	int vw_cnt_arr[5] = { vw_cnt.pgd_count, vw_cnt.p4d_count, vw_cnt.pud_count, vw_cnt.pmd_count, vw_cnt.pte_count };

	if (unlikely(!access_ok(buf, count)))
		return -EFAULT;

	if (count < sizeof(vw_cnt_arr))
		return -EINVAL;

	if (likely(check_copy_size((const void *) &vw_cnt_arr, sizeof(vw_cnt_arr), true)))
		return _copy_to_user((void __user *) buf, (const void *) &vw_cnt_arr, sizeof(vw_cnt_arr));

	return 0;
}

static long vmm_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	long res = -1;

	switch (cmd)
	{
		case MONITOR_PID:
			if (atomic_read(&vmm_pid) == -1)
				res = (atomic_cmpxchg(&vmm_pid, -1, (int) arg) == -1) ? 0 : -1;
			break;
		case PRINT_COUNT:
			pr_info("[VM Monitor] : PID(%d) { PGD: %d, P4D: %d, PUD: %d, PMD: %d, PTD: %d }\n", atomic_read(&vmm_pid),
				vw_cnt.pgd_count, vw_cnt.p4d_count, vw_cnt.pud_count, vw_cnt.pmd_count, vw_cnt.pte_count);
			res = 0;
			break;
		case RESET_PID:
			if (atomic_read(&vmm_pid) == (int) arg)
				res = (atomic_cmpxchg(&vmm_pid, (int) arg, -1) == (int) arg) ? 0 : -1;
			break;
		default:
			break;
	}

	return res;
}


/********************************
 *       GLOBAL FUNCTIONS       *
 ********************************/

int vmm_device_init()
{
	int err = 0;
	struct device *dev;

	vmm_dev_major = __register_chrdev(0, 0, 1, "vm-monitor", &vmm_dev_fops);
	if (vmm_dev_major < 0)
	{
		pr_err("[VM Monitor] : Unable to register the character device\n");
		err = -1;
		goto exit_3;
	}

	vmm_dev_class = class_create(THIS_MODULE, "vm-monitor");
	if (IS_ERR(vmm_dev_class))
	{
		pr_err("[VM Monitor] : Unable to create the device class\n");
		err = -1;
		goto exit_2;
	}

	vmm_dev_class->devnode = vmm_devnode;
	vmm_dev_class->dev_uevent = vmm_uevent;

	dev = device_create(vmm_dev_class, NULL, MKDEV(vmm_dev_major, 0), NULL, "vm-monitor");
	if (IS_ERR(dev))
	{
		pr_err("[VM Monitor] : Unable to create the device\n");
		err = -1;
		goto exit_1;
	}

	goto exit_3;

exit_1:
	class_destroy(vmm_dev_class);
exit_2:
	__unregister_chrdev(vmm_dev_major, 0, 1, "vm-monitor");
exit_3:
	return err;
}

void vmm_device_fini()
{
	device_destroy(vmm_dev_class, MKDEV(vmm_dev_major, 0));

	class_destroy(vmm_dev_class);

	__unregister_chrdev(vmm_dev_major, 0, 1, "vm-monitor");
}