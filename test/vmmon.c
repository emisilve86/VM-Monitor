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

#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include <sys/ioctl.h>
#include <sys/mman.h>


#define MONITOR_PID		3UL
#define RESET_PID		12UL
#define PRINT_COUNT		48UL


/*
 * The following code splits the work into
 * two processes. The parent one open the
 * VMM device to register PID of its child
 * which periodically, randomly mmap/munmap
 * fixed size chunck of virtual memory.
 */
int main(int argc, char *argv[])
{
	int fd;
	pid_t pid;
	unsigned int sec;

	if (argc < 2)
	{
		printf("Error: missing the desired amount of seconds\n");
		return -1;
	}

	if ((sec = (unsigned int) atoi(argv[1])) == 0)
	{
		printf("Error: arguments must be an integer value\n");
		return -1;
	}

	if ((pid = fork()) == -1)
	{
		printf("Error: failed to fork a child process\n");
		return -1;
	}
	else if (pid == 0)
	{
		int i = 0;
		void *addr[10] = { NULL };
		size_t length = (size_t) getpagesize() * 10;

		srand(23);

		while (1)
		{
			usleep(10000);

			if (addr[i] == NULL)
			{
				if ((addr[i] = mmap(NULL, length, PROT_NONE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0)) == MAP_FAILED)
					addr[i] = NULL;
			}
			else
			{
				if (munmap(addr[i], length) == 0)
					addr[i] = NULL;
			}

			i = rand() % 10;
		}

		return 0;
	}
	else
	{
		int count[5];
		ssize_t bytes;

		if ((fd = open("/dev/vm-monitor", O_RDONLY)) == -1)
		{
			printf("Error: failed to open /dev/vm-monitor\n");
			kill(pid, SIGKILL);
			waitpid(pid, NULL, 0);
			return -1;
		}

		if (ioctl(fd, MONITOR_PID, (unsigned long) pid) == -1)
		{
			printf("Error: failed to set pid %d\n", pid);
			close(fd);
			kill(pid, SIGKILL);
			waitpid(pid, NULL, 0);
			return -1;
		}

		sleep(sec);

		if (ioctl(fd, PRINT_COUNT, NULL) == -1)
		{
			printf("Error: failed to print counts to kernel ring buffer\n");
			close(fd);
			kill(pid, SIGKILL);
			waitpid(pid, NULL, 0);
			return -1;
		}

		if ((bytes = read(fd, (void *) &count, sizeof(count))) == -1)
		{
			printf("Error: failed to reset the pid\n");
			close(fd);
			kill(pid, SIGKILL);
			waitpid(pid, NULL, 0);
			return -1;
		}
		else if (bytes != sizeof(count))
		{
			printf("Warning: read a different amount of bytes than requested\n");
		}
		else
		{
			printf("Info: PID(%d) { PGD: %d, P4D: %d, PUD: %d, PMD: %d, PTD: %d }\n",
				pid, count[0], count[1], count[2], count[3], count[4]);
		}

		if (ioctl(fd, RESET_PID, (unsigned long) pid) == -1)
		{
			printf("Error: failed to reset the pid\n");
			close(fd);
			kill(pid, SIGKILL);
			waitpid(pid, NULL, 0);
			return -1;
		}

		close(fd);

		kill(pid, SIGKILL);
		waitpid(pid, NULL, 0);
	}

	return 0;
}