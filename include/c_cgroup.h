#ifndef CONTAINER_CGROUP_H
#define CONTAINER_CGROUP_H

#include <container.h>
#include <c_cgroup/cpu.h>
#include <c_cgroup/cpuacct.h>
#include <c_cgroup/cpuset.h>
#include <c_cgroup/memory.h>

#define AUTO_CGROUP     "./scripts/cgroup_attach"

#define CPU_CGROUP      "my_cpu_cgroup"
#define CPUSET_CGROUP   "my_cpuset_cgroup"
#define CPUACCT_CGROUP  "my_cpuacct_cgroup"
#define MEMORY_CGROUP   "my_memory_cgroup"

/* container cpu limit: 30% */
#define CPU_LIMIT 		30000
/* container memory limit: 64M */
#define MEMORY_LIMIT 	(1 << 26)
/* container cpuset: 0-1, 2 core */
#define CPUSET_LIMIT    0x0001
/* container stack size: 1M */
#define STACK_SIZE 		(1 << 20)

#define cpuset_limit(cpuset, i)	\
({	\
	unsigned long _mask = cpuset;	\
	_mask &= (((~0ul) >> ((7 - i) << 3)) << (i << 3));	\
	_mask >>= (i << 3);	\
	_mask;	\
})
#define cpuset_limit_from(cpuset) cpuset_limit(cpuset, 1)
#define cpuset_limit_to(cpuset) cpuset_limit(cpuset, 0)


#define CONTAINER_DEAMON (0x1 << 0)
#define CONTAINER_EXEC (0x1 << 1)


enum cgroup_index {
	CONTAINER_PID = 1,
	CPU_CGROUP_LIMIT,
	MEMORY_CGROUP_LIMIT,
	CPUSET_CGROUP_LIMIT,
	NULLCGROUP,
};

extern char *auto_cpu_cgroup[];

/* cgroup context */
struct cgroup_context {
	struct cpu_cgrp_ctx         cpu_ctx;
	struct cpuset_cgrp_ctx 	    cpuset_ctx;
	struct mem_cgrp_ctx         memory_ctx;
	struct cpuacct_cgrp_ctx     cpuacct_ctx;
};

int cgroup_init_container_cgrp(pid_t pid);

#endif