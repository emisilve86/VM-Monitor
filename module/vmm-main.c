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

#include <linux/module.h>

#include <linux/printk.h>

#include <linux/mm.h>
#include <linux/mm_types.h>

#include <asm/pgtable.h>
#include <asm/pgtable_types.h>

#include "vmm-device.h"
#include "vmm-walker.h"


/********************************
 *       STATIC FUNCTIONS       *
 ********************************/

static int check_pte_write_defect(struct vm_area_struct *vma, pte_t *pte, unsigned long addr)
{
	return (!(vma->vm_flags & VM_WRITE) && pte_write(*pte)) ? 1 : 0;
}


/********************************
 *       GLOBAL FUNCTIONS       *
 ********************************/

void vmm_ops_init(void)
{
	vw_ops.pgd_entry = NULL;
	vw_ops.p4d_entry = NULL;
	vw_ops.pud_entry = NULL;
	vw_ops.pmd_entry = NULL;
	vw_ops.pte_entry = check_pte_write_defect;
}

__init int vm_monitor_init(void)
{
	pr_info("[VM Monitor] : Module Initialization\n");

	if (vmm_device_init())
		return -1;

	vmm_ops_init();

	if (vmm_walker_init())
	{
		vmm_device_fini();
		return -1;
	}

	pr_info("[VM Monitor] : Initialization Completed\n");

	return 0;
}

__exit void vm_monitor_fini(void)
{
	pr_info("[VM Monitor] : Module Finalization\n");

	vmm_walker_fini();
	vmm_device_fini();

	pr_info("[VM Monitor] : Finalization Completed\n");
}

module_init(vm_monitor_init);
module_exit(vm_monitor_fini);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Emiliano Silvestri <emisilve86@gmail.com>");
MODULE_DESCRIPTION("Virtual Memory Monitor");