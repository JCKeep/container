#include <container.h>
#include <c_cgroup.h>
#include <c_namespace.h>

static char *container[] = {
	"/bin/bash",
	NULL,
};

sigset_t mask;
int container_pid;
int container_exiting;
extern int container_run_deamon;

void signal_handler(int sig)
{
	container_exiting = 1;
	printf("Hello World");
}

void container_init_signal()
{
	sigemptyset(&mask);
	sigaddset(&mask, SIGINT);
	sigaddset(&mask, SIGSTOP);

	sigprocmask(SIG_BLOCK, &mask, NULL);

	signal(SIGINT, signal_handler);
	signal(SIGSTOP, signal_handler);
}

int container_deamon(void *arg)
{
	pid_t pid = getpid();

	container_init_signal();

	if (cgroup_init_container_cgrp(pid) < 0)
		goto fail;

	if (container_init_environ(CONTAINER_DEAMON) < 0)
		goto fail;

	if (namespace_init_container_filesystem(filesystems) < 0)
		goto fail;

	if (namespace_init_container_symlinks(symlinks) < 0)
		goto fail;

	if (deamon() < 0)
		goto fail;

	sigemptyset(&mask);
	for (;;) {
		sigsuspend(&mask);

		if (container_exiting) {
			break;
		}
	}

	return 0;

      fail:
	exit(EXIT_FAILURE);
}

int container_exec(int argc, char *argv[])
{
	int pidfile, status;
	pid_t pid;

	pidfile = open(PIDFILE, O_RDONLY);
	if (read(pidfile, &pid, sizeof(pid_t)) < 0) {
		goto fail;
	}
	close(pidfile);

	if (pid < 0) {
		fprintf(stderr, "containerd not start\n");
		return 0;
	}

	if (cgroup_init_container_cgrp(getpid())) {
		goto fail;
	}

	if (namespace_attach_to_container(pid) < 0) {
		goto fail;
	}

	if (container_init_environ(CONTAINER_EXEC) < 0) {
		goto fail;
	}

	switch ((pid = fork())) {
	case -1:
		perror("fork");
		goto fail;
	case 0:
		if (execvpe(container[0], container, environ) < 0) {
			perror("execvpe container");
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
	return -1;
}

int container_exit()
{
	if (container_get_pid() < 0) {
		return -1;
	}

	kill(container_pid, SIGINT);
	container_exit_pidfile(PIDFILE);
	printf("\033[01;32mcontainer exited\033[0m\n");

	return 0;
}

int main(int argc, char *argv[])
{
	char *stack;
	pid_t pid;

	if (argc == 2 && !strcmp(argv[1], "exec")) {
		return container_exec(argc, argv);
	} else if (argc == 2 && !strcmp(argv[1], "exit")) {
		return container_exit();
	} else if (argc > 2 && !strcmp(argv[1], "run")
		   && !strcmp(argv[2], "--deamon")) {
		container_run_deamon = 1;
		BUG();
	}

	stack = mmap(NULL, STACK_SIZE, PROT_READ | PROT_WRITE, MAP_PRIVATE
		     | MAP_ANONYMOUS, -1, 0);
	if (stack == MAP_FAILED) {
		perror("mmap container stack vm_area");
		goto fail;
	}

	pid = clone(container_deamon, stack + STACK_SIZE,
		    CLONE_NEWCGROUP | CLONE_NEWPID | CLONE_NEWUTS |
		    CLONE_NEWNS | SIGCHLD, NULL);
	if (pid == -1) {
		perror("clone");
		goto fail;
	}

	printf("\033[1;32mcontainer starting\033[0m\ncontainer pid %d\n", pid);
	if (container_run_pidfile(PIDFILE, pid) < 0) {
		goto fail;
	}
	// munmap(stack, STACK_SIZE);

	return 0;

      fail:
	exit(EXIT_FAILURE);
}

int container_init_environ(int flag)
{
	char **p;
	static char *cmd[] = {
		"/usr/bin/cp",
		"scripts/.bashrc",
		ROOT "/root/.bashrc",
		NULL,
	};

	if (setenv("color_prompt", "yes", 1) < 0)
		goto fail;

	if (flag & CONTAINER_EXEC) {
		goto ret;
	}

	/**
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

	/** safety in uts_namespace, we set our own container hostname */
	if (sethostname(CONTAINER_NAME, sizeof(CONTAINER_NAME) - 1) < 0) {
		perror("sethostname");
		goto fail;
	}

      ret:
	return 0;

      fail:
	return -1;
}

int container_run_pidfile(const char *path, pid_t pid)
{
	int pidfile = open(path, O_WRONLY | O_CREAT, 0644);
	if (write(pidfile, &pid, sizeof(pid_t)) < 0) {
		perror("container_run_pidfile");
		return -1;
	}
	close(pidfile);

	return 0;
}

int container_exit_pidfile(const char *path)
{
	int pid = -1;
	int pidfile = open(path, O_WRONLY | O_CREAT, 0644);
	if (write(pidfile, &pid, sizeof(pid_t)) < 0) {
		perror("container_exit_pidfile");
		return -1;
	}
	close(pidfile);

	return 0;
}

int container_get_pid()
{
	if (container_pid) {
		return container_pid;
	}

	int pid, pidfile;

	pidfile = open(PIDFILE, O_RDONLY);
	read(pidfile, &pid, sizeof(pid_t));

	container_pid = pid;
	close(pidfile);

	return pid;
}
