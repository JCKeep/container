#include <container.h>
#include <config.h>
#include <pidfile.h>
#include <c_signal.h>
#include <c_cgroup.h>
#include <c_namespace.h>

extern sigset_t mask;
extern int container_pid;
extern int container_exiting;
extern int container_run_deamon;

extern char run_image[256];
extern struct container_image_builder *cmd;

int container_run(void *arg)
{
	pid_t pid = getpid();
	struct image_mnt image_filesystems[NULLFS];

	memcpy(image_filesystems, filesystems, sizeof(image_filesystems));

	container_init_signal();

	if (config_init(global_config) < 0)
		goto fail;

	if (cgroup_ctx_init(global_cgrp_ctx) < 0)
		goto fail;

	if (namespace_ctx_init(ns_ctx) < 0)
		goto fail;

	if (config_parse(global_config) < 0)
		goto fail;

	if (cgroup_init_container_cgrp(pid) < 0)
		goto fail;

	if (container_init_environ(CONTAINER_DEAMON) < 0)
		goto fail;

	if (container_image_analyze_layer(run_image, cmd) < 0)
		goto fail;

	image_filesystems[ROOTFS].private_data = cmd;
	ns_ctx->mntns.mnts = image_filesystems;
	if (ns_ctx->init(ns_ctx) < 0)
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
	BUG();
	exit(EXIT_FAILURE);
}

int container_exec(int argc, char *argv[])
{
	int pidfile, status;
	pid_t pid;
	const static char *container[] = {
		"/usr/bin/bash",
		NULL,
	};

	pidfile = open(PIDFILE, O_RDONLY);
	if (read(pidfile, &pid, sizeof(pid_t)) < 0) {
		goto fail;
	}
	close(pidfile);

	if (pid < 0) {
		fprintf(stderr, "containerd not start\n");
		return 0;
	}

	if (config_init(global_config) < 0) {
		goto fail;
	}

	if (cgroup_ctx_init(global_cgrp_ctx) < 0) {
		goto fail;
	}

	if (namespace_ctx_init(ns_ctx) < 0) {
		goto fail;
	}

	if (config_parse(global_config) < 0) {
		goto fail;
	}

	if (cgroup_init_container_cgrp(getpid())) {
		goto fail;
	}

	if (ns_ctx->attach(ns_ctx, pid) < 0) {
		goto fail;
	}

	if (container_init_environ(CONTAINER_EXEC) < 0) {
		goto fail;
	}

	printf("\033[01;32menter container\033[0m\n");
	switch ((pid = fork())) {
	case -1:
		perror("fork");
		BUG();
		goto fail;
	case 0:
		if (execvpe(container[0], container, environ) < 0) {
			perror("execvpe container");
			BUG();
			goto fail;
		}
	default:
		waitpid(pid, &status, 0);
		if (WIFSIGNALED(status)) {
			fprintf(stderr,
				"ERROR: container %d has been kill by signal %d\n",
				pid, WTERMSIG(status));
			BUG();
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
	void *container = container_run;

	if (argc == 2 && !strcmp(argv[1], "exec")) {
		return container_exec(argc, argv);
	} else if (argc == 2 && !strcmp(argv[1], "exit")) {
		return container_exit();
	} else if (argc > 2 && !strcmp(argv[1], "build")) {
		return container_build_image(argc, argv);
	}

	if (argc == 3 && !strcmp(argv[1], "run")) {
		// container_run_deamon = 1;
		strcpy(run_image, argv[2]);
	}
#if 0
	if (argc > 3 && !strcmp(argv[1], "run") && !strcmp(argv[2], "images")) {
		cmd->argc = argc - 3;
		cmd->argv = &argv[3];
		cmd->layers = 2;
		strcpy(cmd->images[0],
		       "/root/D/kernel/demo-container/images/ubuntu_redis");
		strcpy(cmd->images[1],
		       "/root/D/kernel/demo-container/images/ubuntu_latest");
		strcpy(cmd->target_image,
		       "/root/D/kernel/demo-container/images/image_build_test");
		container = container_run_command;
	}
#endif

	if (deamon() < 0) {
		BUG();
		goto fail;
	}

	stack = mmap(NULL, STACK_SIZE, PROT_READ | PROT_WRITE, MAP_PRIVATE
		     | MAP_ANONYMOUS, -1, 0);
	if (stack == MAP_FAILED) {
		perror("mmap container stack vm_area");
		goto fail;
	}

	pid = clone(container, stack + STACK_SIZE,
		    CLONE_NEWCGROUP | CLONE_NEWPID | CLONE_NEWUTS |
		    CLONE_NEWNS | SIGCHLD, NULL);
	if (pid == -1) {
		perror("kernel_clone");
		goto fail;
	}

	printf("\033[1;32mcontainer starting\033[0m\ncontainer pid %d\n", pid);
	if (container_run_pidfile(PIDFILE, pid) < 0) {
		goto fail;
	}

	return 0;

      fail:
	exit(EXIT_FAILURE);
}

/**
 * WARNING: BUGS at run cp
 * flag:
 *     CONTAINER_DEAMON on container run
 *     CONTAINER_EXEC on container exec
 */
int container_init_environ(int flag)
{
	char **p;
#ifndef OVERLAY_ROOTFS
	const static char *cmd[] = {
		"/usr/bin/cp",
		"scripts/.bashrc",
		ROOT "/root/.bashrc",
		NULL,
	};
#endif

	if (setenv("color_prompt", "yes", 1) < 0)
		goto fail;

	if (setenv("TZ", "Asia/Shanghai", 1) < 0)
		goto fail;

	if (setenv
	    ("PATH",
	     "/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin",
	     1) < 0)
		goto fail;

	if (flag & CONTAINER_EXEC) {
		goto ret;
	}

	/**
	 * BUGS here
	 *
	 * vfork() use struct completion to promise
	 * child complete or do exec first.
	 *
	 * struct completion {
	 *     unsigned int done;
	 *     struct swait_queue_head wait;
	 * }
	 */
#ifndef OVERLAY_ROOTFS
#if 0
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
#endif
#endif

	for (p = environ; *p; ++p) {
		char *s = strdup(*p);
		if (!s)
			goto fail;

		if (putenv(s) < 0)
			goto fail;
	}

      ret:
	return 0;

      fail:
	BUG();
	return -1;
}
