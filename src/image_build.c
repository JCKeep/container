#include <container.h>
#include <pidfile.h>
#include <c_cgroup.h>
#include <c_namespace.h>
#include <config.h>

char run_image[256];
static struct container_image_builder anon;
struct container_image_builder *cmd = &anon;

/** container run images */
int container_run_command(void *arg)
{
	pid_t pid = getpid();
	struct image_mnt image_filesystems[NULLFS];

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

int container_build_image(int argc, char *argv[])
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
	static struct image_mnt data = {
		.name = "anon",
		.source = DATA,
		.target = ROOT "/data",
		.flags = MS_BIND,
		.mode = 0755,
	};

	fp = fopen("Dockerfile", "r");
	if (container_image_prebuild(fp, cmd, argv[2]) < 0) {
		BUG();
		return -1;
	}

	/* mmap the container stack */
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
			       sizeof(struct image_mnt));
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

int container_image_analyze_layer(const char *image,
				  struct container_image_builder *c)
{
	FILE *fp;
	char buf[256];

	snprintf(buf, sizeof(buf), IMAGES_DIR "/%s/layer", image);
	fp = fopen(buf, "r");

	if (fp == NULL) {
		fprintf(stderr, "no image: %s\n", image);
		return -1;
	}

	fscanf(fp, "%d", &c->layers);

	for (int i = 0; i < c->layers; i++) {
		fscanf(fp, "%s", c->images[i]);
	}

	fclose(fp);
	return 0;
}

int container_image_prebuild(FILE * fp,
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

	if (fpl == NULL) {
		fprintf(stderr, "not found image layer file: %s\n", buf);
		return -1;
	}

	fscanf(fpl, "%d", &c->layers);

	assert(c->layers <= MAX_LAYER);

	for (int i = 0; i < c->layers; i++) {
		fscanf(fpl, "%s", c->images[i]);
	}

	fclose(fpl);
}

int container_image_build_confirm(struct container_image_builder *c,
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

	close(fp);

	if (rust_image_confirm(image_name) < 0) {
		return -1;
	}

	return 0;
}
