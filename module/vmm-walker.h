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

#ifndef __VMM_WALKER_H__
#define __VMM_WALKER_H__


#include <linux/mm_types.h>

#include <asm/pgtable_types.h>


/********************************
 *     GLOBAL PROTOTYPES        *
 ********************************/

int		vmm_walker_init(void);
void	vmm_walker_fini(void);


/********************************
 *      STRUCT DEFINITIONS      *
 ********************************/

typedef struct __vmm_walker_operations__ {
	int (*pgd_entry)(struct vm_area_struct *, pgd_t *, unsigned long);
	int (*p4d_entry)(struct vm_area_struct *, p4d_t *, unsigned long);
	int (*pud_entry)(struct vm_area_struct *, pud_t *, unsigned long);
	int (*pmd_entry)(struct vm_area_struct *, pmd_t *, unsigned long);
	int (*pte_entry)(struct vm_area_struct *, pte_t *, unsigned long);
} vmm_walker_operations;


/********************************
 *     EXPORTED VARIABLES       *
 ********************************/

extern vmm_walker_operations vw_ops;


#endif