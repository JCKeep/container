#include <container.h>
#include <c_cgroup.h>
#include <c_namespace.h>

static char *container[] = {
	"/bin/bash",
	NULL,
};

static int init_container_environ();

int container_main(void *arg)
{
	int status;
	pid_t pid = getpid();

	if (init_container_cgroup(pid) < 0)
		goto fail;

	if (init_container_environ() < 0)
		goto fail;

	if (init_container_filesystem(filesystems) < 0)
		goto fail;

	if (init_container_symlinks(symlinks) < 0)
		goto fail;

	switch ((pid = fork())) {
	case -1:
		perror("fork");
		goto fail;
	case 0:
		if (execvpe(container[0], container, environ) < 0) {
			perror("execv container");
			goto fail;
		}
	default:
		waitpid(pid, &status, 0);
		if (WIFSIGNALED(status)) {
			fprintf(stderr,
				"ERROR: container %d has been kill by signal %d\n",
				pid, WTERMSIG(status));
			goto fail;
		}
	}

	return 0;

      fail:
	exit(EXIT_FAILURE);
}

int main()
{
	int status;
	char *stack;
	pid_t pid;

	stack = mmap(NULL, STACK_SIZE, PROT_READ | PROT_WRITE, MAP_PRIVATE
		     | MAP_ANONYMOUS, -1, 0);
	if (stack == MAP_FAILED) {
		perror("mmap");
		goto fail;
	}

	pid = clone(container_main, stack + STACK_SIZE,
		    CLONE_NEWCGROUP | CLONE_NEWPID | CLONE_NEWUTS |
		    CLONE_NEWNS | SIGCHLD, NULL);
	if (pid == -1) {
		perror("clone");
		goto fail;
	}

	printf("container pid %d\n", pid);

	waitpid(pid, &status, 0);
	if (WIFSIGNALED(status)) {
		fprintf(stderr,
			"ERROR: child %d has been kill by signal %d\n",
			pid, WTERMSIG(status));
	}

	munmap(stack, STACK_SIZE);
	printf("\033[01;32mcontainer exited\033[0m\n");

	return 0;

      fail:
	exit(EXIT_FAILURE);
}

int init_container_environ()
{
	char **p;
	char *cmd[] = {
		"/usr/bin/cp",
		"scripts/.bashrc",
		ROOT "/root/.bashrc",
		NULL,
	};

	if (setenv("color_prompt", "yes", 1) < 0)
		goto fail;
    
	/*
	 * vfork() use struct completion to promise
	 * child complete or do exec first.
	 * 
	 * struct completion {
	 *     unsigned int done;
	 *     struct swait_queue_head wait;
	 * }
	 */
	switch (vfork()) {
	case -1:
		perror("fork");
		goto fail;
	case 0:
		if (execv(cmd[0], cmd) < 0)
			goto fail;
	default:
		break;
	}

	for (p = environ; *p; ++p) {
		char *s = strdup(*p);
		if (!s)
			goto fail;

		if (putenv(s) < 0)
			goto fail;
	}

	/* safety in uts_namespace, we set our own container hostname */
	if (sethostname(CONTAINER_NAME, sizeof(CONTAINER_NAME) - 1) < 0) {
		perror("sethostname");
		goto fail;
	}

	return 0;

      fail:
	return -1;
}
