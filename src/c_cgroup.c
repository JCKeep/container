#include <c_cgroup.h>

char *auto_cpu_cgroup[] = {
	[0] = AUTO_CGROUP,
	[NULLCGROUP] = NULL,
};

int init_container_cgroup(pid_t pid)
{
	int  _pid;
	char spid[16], cpu_limit[16], mem_limit[16], cpuset_limit[16];

	switch ((_pid = fork())) {
	case -1:
		perror("fork");
		return -1;
	case 0:
		sprintf(spid, "%d", pid);
		auto_cpu_cgroup[CONTAINER_PID] = spid;
		sprintf(cpu_limit, "%d", CPU_LIMIT);
		auto_cpu_cgroup[CPU_CGROUP_LIMIT] = cpu_limit;
		sprintf(mem_limit, "%d", MEMORY_LIMIT);
		auto_cpu_cgroup[MEMORY_CGROUP_LIMIT] = mem_limit;
		sprintf(cpuset_limit, "%ld-%ld",
			cpuset_limit_from(CPUSET_LIMIT),
			cpuset_limit_to(CPUSET_LIMIT));
		auto_cpu_cgroup[CPUSET_CGROUP_LIMIT] = cpuset_limit;

		if (execvpe(auto_cpu_cgroup[0], auto_cpu_cgroup, environ) < 0) {
			perror("cgroup");
			return -1;
		}
	default:
		waitpid(_pid, NULL, 0);
		break;
	}

	return 0;
}
