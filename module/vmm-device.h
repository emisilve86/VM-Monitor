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

#ifndef __VMM_DEVICE_H__
#define __VMM_DEVICE_H__


#include <linux/types.h>


#define MONITOR_PID		3UL
#define RESET_PID		12UL
#define PRINT_COUNT		48UL


/********************************
 *     GLOBAL PROTOTYPES        *
 ********************************/

int		vmm_device_init(void);
void	vmm_device_fini(void);


/********************************
 *      STRUCT DEFINITIONS      *
 ********************************/

typedef struct __vmm_walker_counter__ {
	int pgd_count;
	int p4d_count;
	int pud_count;
	int pmd_count;
	int pte_count;
} vmm_walker_counter;


/********************************
 *     EXPORTED VARIABLES       *
 ********************************/

extern atomic_t vmm_pid;
extern vmm_walker_counter vw_cnt;


#endif