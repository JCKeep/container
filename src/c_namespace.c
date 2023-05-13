#include <c_namespace.h>

static struct namespace_context ____ctx;
struct namespace_context *ns_ctx = &____ctx;

struct ns_ctx_modules global_ns_ctx_modules[] = {
	{
	 .name = "mntns",
	 .module = MNTNS,
	 .offset = offsetof(struct namespace_context, mntns),
	  },
	{
	 .name = "utsns",
	 .module = UTSNS,
	 .offset = offsetof(struct namespace_context, utsns),
	  },
	{ }
};

#define DEFINE_NSMAP(_name, _flag) { .name = _name, .flags = _flag }

const static struct namespace_map ns_map[] = {
	DEFINE_NSMAP("mnt", CLONE_NEWNS),
	DEFINE_NSMAP("uts", CLONE_NEWUTS),
	DEFINE_NSMAP("pid", CLONE_NEWPID),
	DEFINE_NSMAP("user", CLONE_NEWUSER),
	DEFINE_NSMAP("cgroup", CLONE_NEWCGROUP),
	DEFINE_NSMAP("ipc", CLONE_NEWIPC),
	DEFINE_NSMAP("net", CLONE_NEWNET),
	DEFINE_NSMAP("pid_for_children", CLONE_NEWPID),
	/* tag for the end */
	DEFINE_NSMAP(NULL, 0),
};

/**
 * only few namespace, for loop is enough
 */
static unsigned long namespace_find_flags(const char *name)
{
	const struct namespace_map *ns = ns_map;

	while (ns->name) {
		if (strcmp(name, ns->name) == 0) {
			return ns->flags;
		}

		ns++;
	}

	return 0;
}

#if 0
static int write_map(pid_t pid, const char *path, uid_t inside_id,
		     uid_t outside_id, size_t count)
{
	int fd;
	char buf[1024];

	snprintf(buf, sizeof(buf), "%d %d %zu\n", inside_id, outside_id, count);
	if ((fd = open(path, O_WRONLY)) == -1) {
		return -1;
	}

	if (write(fd, buf, strlen(buf)) == -1) {
		close(fd);
		return -1;
	}

	close(fd);
	return 0;
}

static int write_uid_map(pid_t pid, uid_t inside_id, uid_t outside_id,
			 size_t count)
{
	char path[128];

	snprintf(path, sizeof(path), "/proc/%d/uid_map", pid);

	return write_map(pid, path, inside_id, outside_id, count);
}

static int write_gid_map(pid_t pid, uid_t inside_id, uid_t outside_id,
			 size_t count)
{
	char path[128];

	snprintf(path, sizeof(path), "/proc/%d/gid_map", pid);

	return write_map(pid, path, inside_id, outside_id, count);
}

int namespace_init_user_ns(pid_t pid, uid_t uid, gid_t gid)
{
	if (write_uid_map(pid, 0, uid, 1) < 0) {
		perror("write_uid_map");
		return -1;
	}

	if (write_gid_map(pid, 0, gid, 1) < 0) {
		perror("write_gid_map");
		return -1;
	}

	return 0;
}

int namespace_preinit_net_ns()
{
	const static char *cmd[] = {
		"./scripts/net_namespace_init",
		NULL,
	};

	switch (vfork()) {
	case -1:
		BUG();
		goto fail;
	case 0:
		if (execv(cmd[0], cmd) < 0) {
			BUG();
			goto fail;
		}
	default:
		break;
	}

	return 0;

      fail:
	return -1;
}

int namespace_attach_net_ns(pid_t pid)
{
	const static char *cmd[] = {
		"./scripts/net_namespace_attach",
		NULL,
	};

	int fd = open("/var/run/netns/" NET_NAMESPACE, O_RDONLY);
	if (fd < 0) {
		perror("open my_net_ns");
		goto fail;
	}

	if (setns(fd, CLONE_NEWNET) < 0) {
		perror("setns net");
		goto fail;
	}

	close(fd);

	switch (vfork()) {
	case -1:
		BUG();
		goto fail;
	case 0:
		if (execv(cmd[0], cmd) < 0) {
			BUG();
			goto fail;
		}
	default:
		break;
	}

	return 0;

      fail:
	return -1;
}
#endif

static int namespace_init(struct namespace_context *ctx)
{
	int ret = 0;
	char *base = ctx;
	struct ns_module *module;
	struct ns_ctx_modules *it = global_ns_ctx_modules;

	while (it->name) {
		module = base + it->offset;

		ret = module->init(ctx);
		if (ret < 0) {
			BUG();
			return -1;
		}

		it++;
	}

	return 0;
}

/**
 * attach current to a container with pid
 */
static int namespace_attach_to_container(struct namespace_context *ctx,
					 pid_t pid)
{
	int fd;
	DIR *dir;
	char ns_path[1024];
	struct dirent *ns = NULL;
	unsigned long flag = 0;

	snprintf(ns_path, sizeof(ns_path), "/proc/%d/ns/", pid);
	dir = opendir(ns_path);

	while ((ns = readdir(dir))) {
		flag = namespace_find_flags(ns->d_name);
		if (!flag) {
			continue;
		}
#ifdef NO_NSUSER
		/* if do mount without nsuser */
		if (flag == CLONE_NEWUSER) {
			continue;
		}
#endif

		snprintf(ns_path, sizeof(ns_path), "/proc/%d/ns/%s", pid,
			 ns->d_name);
		if ((fd = open(ns_path, O_RDONLY)) < 0) {
			perror("open ns");
			goto fail;
		}

		if (setns(fd, flag) < 0) {
			perror("setns");
			goto fail;
		}

		close(fd);
	}

	if (chroot(ROOT) < 0) {
		perror("chroot");
		goto fail;
	}

	if (chdir("/root") < 0) {
		perror("chdir");
		goto fail;
	}

	closedir(dir);

	return 0;

      fail:
	BUG();
	return -1;
}

int namespace_ctx_init(struct namespace_context *ctx)
{
	memset(ctx, 0, sizeof(*ctx));

	ctx->init = namespace_init;
	ctx->attach = namespace_attach_to_container;

	/* if add a new namespace, init below */
	if (mntns_ctx_init(&ctx->mntns) < 0) {
		return -1;
	}

	if (utsns_ctx_init(&ctx->utsns) < 0) {
		return -1;
	}

	return 0;
}
