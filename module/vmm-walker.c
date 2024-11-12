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

#include <linux/version.h>

#include <linux/err.h>
#include <linux/delay.h>
#include <linux/printk.h>

#include <linux/mm.h>
#include <linux/mm_types.h>
#include <linux/hugetlb_inline.h>

#include <asm/pgtable.h>
#include <asm/pgtable_types.h>

#include <linux/rwsem.h>
#include <linux/sched.h>
#include <linux/kthread.h>
#include <linux/sched/mm.h>
#include <linux/sched/task.h>

#include "vmm-device.h"
#include "vmm-walker.h"


/********************************
 *       STATIC VARIABLES       *
 ********************************/

static struct task_struct *vmm_thread = NULL;


/********************************
 *       GLOBAL VARIABLES       *
 ********************************/

vmm_walker_operations vw_ops = { 0 };


/********************************
 *       STATIC FUNCTIONS       *
 ********************************/

static inline __attribute__((always_inline))
void cond_resched_sem(struct rw_semaphore *sem, bool check)
{
	if (check && !(rwsem_is_contended(sem) || should_resched(PREEMPT_LOCK_OFFSET)))
		return;

	up_read(sem);
	cond_resched();
	down_read(sem);
}

static int vmm_walk_pte_range(struct vm_area_struct *vma, pmd_t *pmd, unsigned long addr, unsigned long end)
{
	int ret;
	pte_t *pte;

	ret = 0;
	pte = pte_offset_map(pmd, addr);

	for (;;)
	{
		if (!pte_none(*pte) && vw_ops.pte_entry)
			ret += vw_ops.pte_entry(vma, pte, addr);

		if (addr >= end - PAGE_SIZE)
			break;
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5,8,0)
		cond_resched_sem(&vma->vm_mm->mmap_lock, true);
#else
		cond_resched_sem(&vma->vm_mm->mmap_sem, true);
#endif

		addr += PAGE_SIZE;
		pte++;
	}

	pte_unmap(pte);

	if (vw_ops.pte_entry)
		vw_cnt.pte_count += ret;

	return ret;
}

static int vmm_walk_pmd_range(struct vm_area_struct *vma, pud_t *pud, unsigned long addr, unsigned long end)
{
	int ret;
	pmd_t *pmd;
	unsigned long next;

	ret = 0;
	pmd = pmd_offset(pud, addr);

	do
	{
		next = pmd_addr_end(addr, end);

		if (pmd_none(*pmd))
			continue;

		if (vw_ops.pmd_entry)
			ret += vw_ops.pmd_entry(vma, pmd, addr);

		ret += vmm_walk_pte_range(vma, pmd, addr, next);

#if LINUX_VERSION_CODE >= KERNEL_VERSION(5,8,0)
		cond_resched_sem(&vma->vm_mm->mmap_lock, false);
#else
		cond_resched_sem(&vma->vm_mm->mmap_sem, false);
#endif
	}
	while (pmd++, addr = next, addr != end);

	if (vw_ops.pmd_entry)
		vw_cnt.pmd_count += ret;

	return ret;
}

static int vmm_walk_pud_range(struct vm_area_struct *vma, p4d_t *p4d, unsigned long addr, unsigned long end)
{
	int ret;
	pud_t *pud;
	unsigned long next;

	ret = 0;
	pud = pud_offset(p4d, addr);

	do
	{
		next = pud_addr_end(addr, end);

		if (pud_none(*pud))
			continue;

		if (vw_ops.pud_entry)
			ret += vw_ops.pud_entry(vma, pud, addr);

		ret += vmm_walk_pmd_range(vma, pud, addr, next);

#if LINUX_VERSION_CODE >= KERNEL_VERSION(5,8,0)
		cond_resched_sem(&vma->vm_mm->mmap_lock, false);
#else
		cond_resched_sem(&vma->vm_mm->mmap_sem, false);
#endif
	}
	while (pud++, addr = next, addr != end);

	if (vw_ops.pud_entry)
		vw_cnt.pud_count += ret;

	return ret;
}

static int vmm_walk_p4d_range(struct vm_area_struct *vma, pgd_t *pgd, unsigned long addr, unsigned long end)
{
	int ret;
	p4d_t *p4d;
	unsigned long next;

	ret = 0;
	p4d = p4d_offset(pgd, addr);

	do
	{
		next = p4d_addr_end(addr, end);

		if (p4d_none(*p4d))
			continue;

		if (vw_ops.p4d_entry)
			ret += vw_ops.p4d_entry(vma, p4d, addr);

		ret += vmm_walk_pud_range(vma, p4d, addr, next);

#if LINUX_VERSION_CODE >= KERNEL_VERSION(5,8,0)
		cond_resched_sem(&vma->vm_mm->mmap_lock, false);
#else
		cond_resched_sem(&vma->vm_mm->mmap_sem, false);
#endif
	}
	while (p4d++, addr = next, addr != end);

	if (vw_ops.p4d_entry)
		vw_cnt.p4d_count += ret;

	return ret;
}

static int vmm_walk_pgd_range(struct vm_area_struct *vma, unsigned long addr, unsigned long end)
{
	int ret;
	pgd_t *pgd;
	unsigned long next;

	ret = 0;
	pgd = pgd_offset(vma->vm_mm, addr);

	do
	{
		next = pgd_addr_end(addr, end);

		if (pgd_none(*pgd))
			continue;

		if (vw_ops.pgd_entry)
			ret += vw_ops.pgd_entry(vma, pgd, addr);

		ret += vmm_walk_p4d_range(vma, pgd, addr, next);

#if LINUX_VERSION_CODE >= KERNEL_VERSION(5,8,0)
		cond_resched_sem(&vma->vm_mm->mmap_lock, false);
#else
		cond_resched_sem(&vma->vm_mm->mmap_sem, false);
#endif
	}
	while (pgd++, addr = next, addr != end);

	if (vw_ops.pgd_entry)
		vw_cnt.pgd_count += ret;

	return ret;
}

static int vm_monitor_thread(void *data)
{
	int ret;
	int pid;

	bool should_stop;

	struct task_struct *task;
	struct vm_area_struct *vma;

	while (!(should_stop = kthread_should_stop()))
	{
		if ((pid = atomic_read(&vmm_pid)) == -1)
			goto kt_must_sleep;

		if ((task = pid_task(find_vpid(pid), PIDTYPE_PID)) == NULL)
			goto kt_must_sleep;

		ret = 0;

		/*
		 * Increment the task's usage reference counter
		 */
		get_task_struct(task);

		if (task->mm == NULL)
			goto kt_unref_task;

		/*
		 * Make sure that task's mm will not get freed
		 * even after the owning task exits
		 */
		mmgrab(task->mm);

		/*
		 * Increase the number of users including userspace
		 * in case it was not already zero
		 */
		if (!mmget_not_zero(task->mm))
			goto kt_unref_mm_count;

		/*
		 * Take the mm's RW semaphore to protect read
		 * accesses
		 */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5,8,0)
		down_read(&task->mm->mmap_lock);
#else
		down_read(&task->mm->mmap_sem);
#endif

		if ((vma = task->mm->mmap) == NULL)
			goto kt_unlock_unref_mm_user;

		do
		{
			if ((should_stop = kthread_should_stop()))
				break;

			/*
			 * Memory areas not controlled by the normal
			 * allocation system
			 */
			if (vma->vm_flags & VM_PFNMAP)
				continue;

			/*
			 * Currently huge pages are not explored
			 */
			if (is_vm_hugetlb_page(vma))
				continue;

			ret += vmm_walk_pgd_range(vma, vma->vm_start, vma->vm_end);
		}
		while ((vma = vma->vm_next) != NULL);

kt_unlock_unref_mm_user:
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5,8,0)
		up_read(&task->mm->mmap_lock);
#else
		up_read(&task->mm->mmap_sem);
#endif

		mmput(task->mm);
kt_unref_mm_count:
		mmdrop(task->mm);
kt_unref_task:
		put_task_struct(task);

		if (should_stop)
			break;

kt_must_sleep:
		msleep(1000);
	}

	return 0;
}


/********************************
 *       GLOBAL FUNCTIONS       *
 ********************************/

int vmm_walker_init()
{
	int err = 0;

	vmm_thread = kthread_create(vm_monitor_thread, NULL, "vm-monitor");
	if (IS_ERR(vmm_thread))
	{
		pr_err("[VM Monitor] : Unable to create the kernel thread\n");
		err = -1;
		goto exit_1;
	}

	if (wake_up_process(vmm_thread) == 0)
		pr_warn("[VM Monitor] : The kernel thread was already running\n");

exit_1:
	return err;
}

void vmm_walker_fini()
{
	if (kthread_stop(vmm_thread) != 0)
		pr_warn("[VM Monitor] : Unable to stop the kernel thread or it never woke up\n");
}