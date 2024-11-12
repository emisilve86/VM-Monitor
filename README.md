# VM-Monitor

## Description

This Linux kernel module provides the users with a solution to inspect both VM area structures and page table entries of target processes. It allows to perform custom checks at each level of the page table directory, such as verifying whether specific bit of `PGD`, `P4D`, `PUD`, `PMD` and `PTE` entries are set or not, or if their states are consistent with respect to those specified in the `vm_area_struct` instances they belong to.

It provides same logic of the kernel's page-walker implemented in `/mm/pagewalk.c` but some additional precautions not to incur the risk of having the `task_struct` and `mm_struct` of the target process freed while the walker is running, and preventing the CPU from stalling by mean of periodic calls to `cond_resched` immediately-after/just-before releasing/acquiring the MM's RW-semaphore.

Once a process has been identified as the target, and its PID passed to the special devive `/dev/vm-monitor`, a kernel thread, specifically spawned to perform page-walking, begins its exploration thought the page table directory.