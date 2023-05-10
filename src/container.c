#include <container.h>
#include <config.h>
#include <c_cgroup.h>
#include <c_namespace.h>

sigset_t mask;
extern int container_pid;
extern int container_exiting;
extern int container_run_deamon;

void signal_handler(int sig)
{
	container_exiting = 1;
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

static char run_image[256];
static struct container_image_builder anon;
static struct container_image_builder *cmd = &anon;

static int build_container_image_env(const char *image,
				     struct container_image_builder *c)
{
	FILE *fp;
	char buf[256];

	snprintf(buf, sizeof(buf), IMAGES_DIR "/%s/layer", image);
	fp = fopen(buf, "r");

	fscanf(fp, "%d", &c->layers);

	for (int i = 0; i < c->layers; i++) {
		fscanf(fp, "%s", c->images[i]);
	}

	fclose(fp);
	return 0;
}

int container_run(void *arg)
{
	pid_t pid = getpid();
	struct mount_args image_filesystems[NULLFS];

	memcpy(image_filesystems, filesystems, sizeof(image_filesystems));

	container_init_signal();

	if (config_init(global_config) < 0)
		goto fail;

	if (cgroup_ctx_init(global_cgrp_ctx) < 0)
		goto fail;

	if (config_parse(global_config) < 0)
		goto fail;

	if (cgroup_init_container_cgrp(pid) < 0)
		goto fail;

	if (container_init_environ(CONTAINER_DEAMON) < 0)
		goto fail;

	if (build_container_image_env(run_image, cmd) < 0)
		goto fail;

	image_filesystems[ROOTFS].private_data = cmd;
	if (namespace_init_container_filesystem(image_filesystems, NULLFS) < 0)
		goto fail;

	if (namespace_init_container_symlinks(symlinks) < 0)
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

	if (config_parse(global_config) < 0) {
		goto fail;
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

/** container run images */
static int container_run_command(void *arg)
{
	pid_t pid = getpid();
	struct mount_args image_filesystems[NULLFS];

	memcpy(image_filesystems, filesystems, sizeof(image_filesystems));

	if (config_init(global_config) < 0)
		goto fail;

	if (cgroup_ctx_init(global_cgrp_ctx) < 0)
		goto fail;

	if (config_parse(global_config) < 0)
		goto fail;

	if (cgroup_init_container_cgrp(pid) < 0)
		goto fail;

	if (container_init_environ(CONTAINER_DEAMON) < 0)
		goto fail;

	image_filesystems[ROOTFS].private_data = cmd;
	if (namespace_init_container_filesystem(image_filesystems, NULLFS) < 0)
		goto fail;

	if (namespace_init_container_symlinks(symlinks) < 0)
		goto fail;

	switch ((pid = fork())) {
	case -1:
		perror("container_run_command fork");
		goto fail;
	case 0:
		if (execv(cmd->argv[0], cmd->argv) < 0) {
			perror("container_run_command execv images");
			goto fail;
		}
	default:
		waitpid(pid, NULL, 0);
		break;
	}

	return 0;

      fail:
	BUG();
	exit(EXIT_FAILURE);
}

static void container_image_prebuild(FILE * fp,
				     struct container_image_builder *c,
				     const char *image)
{
	char buf[256];
	FILE *fpl;

	// fp = fopen("Dockerfile", "r");
	fgets(buf, sizeof(buf), fp);
	sscanf(buf, "FROM %s", c->images[0]);
	snprintf(buf, sizeof(buf), IMAGES_DIR "/%s/layer", c->images[0]);
	snprintf(c->target_image, sizeof(c->target_image), IMAGES_DIR "/%s",
		 image);

	fpl = fopen(buf, "r");
	fscanf(fpl, "%d", &c->layers);
	for (int i = 0; i < c->layers; i++) {
		fscanf(fpl, "%s", c->images[i]);
	}

	fclose(fpl);
}

static int container_image_build_confirm(struct container_image_builder *c,
					 const char *image_name)
{
	char buf[256];
	FILE *fp;

	snprintf(buf, sizeof(buf), "%s/layer", c->target_image);
	fp = fopen(buf, "w");
	fprintf(fp, "%d\n%s\n", c->layers + 1, c->target_image);

	for (int i = 0; i < c->layers; i++) {
		fprintf(fp, "%s\n", c->images[i]);
	}

	if (rust_image_confirm(image_name) < 0) {
		return -1;
	}

	close(fp);
	return 0;
}

static int container_build_image(int argc, char *argv[])
{
	enum dockerfile_cmd {
		FROM = 0,
		RUN,
		COPY,
	} state = 0;
	int eof, fd;
	char *stack, buf[256], path[256], *p, *item;
	FILE *fp;
	pid_t pid;
	struct stat st;
	static struct mount_args data = {
		.name = "anon",
		.source = DATA,
		.target = ROOT "/data",
		.flags = MS_BIND,
		.mode = 0755,
	};

	fp = fopen("Dockerfile", "r");
	container_image_prebuild(fp, cmd, argv[2]);

	stack = mmap(NULL, STACK_SIZE, PROT_READ | PROT_WRITE, MAP_PRIVATE
		     | MAP_ANONYMOUS, -1, 0);
	if (stack == MAP_FAILED) {
		perror("mmap container stack vm_area");
		goto fail;
	}

	cmd->argc = 0;
	cmd->argv = malloc(24 * sizeof(char *));
      next:
	while ((eof = fgets(buf, sizeof(buf), fp))) {
		p = buf;
		item = NULL;
		while (isblank(*p)) {
			p++;
		}

		dbg(p);

		if (!strncmp("RUN", p, 3)) {
			state = RUN;
			p += 3;
			goto run_;
		} else if (!strncmp("COPY", p, 4)) {
			state = COPY;
			p += 4;
			goto copy_;
		} else {
			state = 0;
			continue;
		}

	      run_:
		cmd->argc = 0;
		item = strtok(p, " \t\n");
		while (item) {
			cmd->argv[cmd->argc++] = strdup(item);
			item = strtok(NULL, " \t\n");
		}

		cmd->argv[cmd->argc] = NULL;
		break;

	      copy_:
		cmd->argc = 0;
		memcpy(&filesystems[MOUNT_1], &data, sizeof(data));

		cmd->argv[cmd->argc++] = strdup("/usr/bin/cp");
		item = strtok(p, " \t\n");
		while (item) {
			cmd->argv[cmd->argc++] = item;
			item = strtok(NULL, " \t\n");
		}
		cmd->argv[cmd->argc] = NULL;

		for (int i = 1; i < cmd->argc - 1; i++) {
			snprintf(path, sizeof(path), "/data/%s", cmd->argv[i]);
			cmd->argv[i] = strdup(path);
		}
		cmd->argv[cmd->argc - 1] = strdup(cmd->argv[cmd->argc - 1]);

		break;
	}

	if (!eof) {
		goto end;
	}
#if 0
      again:
	cmd->argc = 0;
	cmd->argv = malloc(24 * sizeof(char *));
	while ((eof = fscanf(fp, "%s", buf)) != EOF) {
		if (!strcmp("RUN", buf)) {
			if (cmd->argc) {
				break;
			} else {
				continue;
			}
		}

		cmd->argv[cmd->argc++] = strdup(buf);
		dbg(buf);
	}
	cmd->argv[cmd->argc] = NULL;
#endif
	pid = clone(container_run_command, stack + STACK_SIZE,
		    CLONE_NEWCGROUP | CLONE_NEWPID | CLONE_NEWUTS |
		    CLONE_NEWNS | SIGCHLD, NULL);
	if (pid == -1) {
		perror("kernel_clone");
		goto fail;
	}

	waitpid(pid, NULL, 0);

	for (int i = 0; i < cmd->argc; i++) {
		free(cmd->argv[i]);
	}

	if (eof) {
		if (state == COPY) {
			memset(&filesystems[MOUNT_1], 0,
			       sizeof(struct mount_args));
		}

		goto next;
	}
      end:
	if (container_image_build_confirm(cmd, argv[2]) < 0) {
		return -1;
	}

	free(cmd->argv);
	close(fp);

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

	/** safety in uts_namespace, we set our own container hostname */
	if (sethostname(CONTAINER_NAME, sizeof(CONTAINER_NAME) - 1) < 0) {
		perror("sethostname");
		goto fail;
	}

      ret:
	return 0;

      fail:
	BUG();
	return -1;
}
