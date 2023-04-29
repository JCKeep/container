#include <container.h>

#ifndef CONTAINER_CGROUP_H
#define CONTAINER_CGROUP_H

#define AUTO_CGROUP     "./scripts/cgroup_attach"

#define CPU_CGROUP      "my_cpu_cgroup"
#define CPUSET_CGROUP   "my_cpuset_cgroup"
#define CPUACCT_CGROUP  "my_cpuacct_cgroup"
#define MEMORY_CGROUP   "my_memory_cgroup"

/* container cpu limit: 30% */
#define CPU_LIMIT 		30000
/* container memory limit: 128M */
#define MEMORY_LIMIT 	(1 << 27)
/* container cpuset: 0-1, one core */
#define CPUSET_LIMIT    0x0001
/* container stack size: 2M */
#define STACK_SIZE 		(1 << 21)

#define cpuset_limit(cpuset, i)	\
({	\
	unsigned long _mask = cpuset;	\
	_mask &= (((~0ul) >> ((7 - i) << 3)) << (i << 3));	\
	_mask >>= (i << 3);	\
	_mask;	\
})
#define cpuset_limit_from(cpuset) cpuset_limit(cpuset, 1)
#define cpuset_limit_to(cpuset) cpuset_limit(cpuset, 0)

enum cgroup_index {
	CONTAINER_PID = 1,
	CPU_CGROUP_LIMIT,
	MEMORY_CGROUP_LIMIT,
	CPUSET_CGROUP_LIMIT,
	NULLCGROUP,
};

extern char *auto_cpu_cgroup[];

int init_container_cgroup(pid_t pid);
int container_main(void *arg);

#endif