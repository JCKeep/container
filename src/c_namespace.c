#include <c_namespace.h>

struct namespace_map {
	char *name;
	unsigned long flags;

	/* todo */
	int (*ns_handler)(struct namespace_map * ns);
};

#define DEFINE_NSMAP(_name, _flag) { .name = _name, .flags = _flag }

#ifdef OVERLAY_ROOTFS

static int overlayfs_handler(struct mount_args __unused * fs)
{
	int __unused ret;
	DIR *dir = NULL;

	if (!(dir = opendir(LOWER_DIR))) {
		fprintf(stderr, "lowerdir not exist\n");
		return -1;
	} else {
		closedir(dir);
	}

	if (!(dir = opendir(UPPER_DIR))) {
		ret = mkdir(UPPER_DIR, 0755);
	} else {
		dbg("dir exist");
		closedir(dir);
	}

	if (!(dir = opendir(WORK_DIR))) {
		ret = mkdir(WORK_DIR, 0755);
	} else {
		dbg("dir exist");
		closedir(dir);
	}

	dbg("overlay");

	return 0;
}
#endif

/** minimal container filesystems to mount */
const struct mount_args filesystems[] = {
#ifdef OVERLAY_ROOTFS
	[ROOTFS] = {
		    .name = "rootfs",
		    .source = "overlay",
		    .target = ROOT,
		    .filesystemtype = "overlay",
		    .flags = MS_MGC_VAL,
		    .mode = 0755,
		    .data =
		    "lowerdir=" LOWER_DIR ",upperdir=" UPPER_DIR ",workdir="
		    WORK_DIR,
		    .handler = overlayfs_handler,
		     },
	[PROCFS] = {
		    .name = "procfs",
		    .source = "proc",
		    .target = ROOT "/proc",
		    .filesystemtype = "proc",
		    .mode = 0555,
		     },
	[SYSFS] = {
		   .name = "sysfs",
		   .source = "sysfs",
		   .target = ROOT "/sys",
		   .filesystemtype = "sysfs",
		   .mode = 0555,
		    },
#else
	[ROOTFS] = {
		    .name = "rootfs",
		    .source = "/dev/null",
		    .target = ROOT,
		    .filesystemtype = "tmpfs",
		    .mode = 0755,
		     },
	[PROCFS] = {
		    .name = "procfs",
		    .source = "proc",
		    .target = ROOT "/proc",
		    .filesystemtype = "proc",
		    .mode = 0555,
		     },
	[DEVFS] = {
		   .name = "devfs",
		   .source = "udev",
		   .target = ROOT "/dev",
		   .filesystemtype = "devtmpfs",
		   .mode = 0755,
		    },
	[SYSFS] = {
		   .name = "sysfs",
		   .source = "sysfs",
		   .target = ROOT "/sys",
		   .filesystemtype = "sysfs",
		   .mode = 0555,
		    },
	[USRFS] = {
		   .name = "usrfs",
		   .source = "/usr",
		   .target = ROOT "/usr",
		   .flags = MS_BIND,
		   .mode = 0555,
		    },
	[ETCFS] = {
		   .name = "etcfs",
		   .source = "/etc",
		   .target = ROOT "/etc",
		   .flags = MS_BIND,
		   .mode = 0555,
		    },
	/* no need ? */
	[VARFS] = {
		   .name = "varfs",
		   .source = "/var",
		   .target = ROOT "/var",
		   .flags = MS_BIND,
		   .mode = 0555,
		    },
#endif
	/* tag for the end */
	[NULLFS] = { },
};

/**  */
const char *symlinks[] = {
#ifndef OVERLAY_ROOTFS
	/* from    ->      to */
	"/usr/bin", "/bin",
	"/usr/lib", "/lib",
	"/usr/lib64", "/lib64",
	"/usr/sbin", "/sbin",
#endif
	/* tag for the end */
	NULL, NULL,
};

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

int namespace_init_container_symlinks(const char *links[])
{
	int i;

	for (i = 0; links[i]; i += 2) {
		if (symlink(links[i], links[i + 1]) < 0) {
			perror("symlink");
			return -1;
		}
	}

	return 0;
}

/* mount given filesystems */
int namespace_init_container_filesystem(const struct mount_args *args)
{
	int ret = 0;
	const struct mount_args *fs = args, *end = &args[NULLFS];

	while (fs != end) {
		if (!fs->target || !fs->source) {
			goto next;
		}

		if (mkdir(fs->target, fs->mode) < 0) {
#ifndef OVERLAY_ROOTFS
			fprintf(stderr, "mkdir %s: %s\n", fs->target,
				strerror(errno));
			goto fail;
#endif
		}

		if (fs->handler) {
			ret = fs->handler(fs);
		}

		if (ret < 0) {
			goto fail;
		}

		if (mount
		    (fs->source, fs->target, fs->filesystemtype, fs->flags,
		     fs->data) < 0) {
			fprintf(stderr, "mount %s: %s\n", fs->name,
				strerror(errno));
			goto fail;
		}

	      next:
		fs++;
	}

	if (chroot(ROOT) < 0) {
		perror("chroot");
		goto fail;
	}

	if (mkdir("/root", 0755) < 0) {
#ifndef OVERLAY_ROOTFS
		perror("mkdir /root");
		goto fail;
#endif
	}

	if (chdir("/root") < 0) {
		perror("chdir");
		goto fail;
	}

	return 0;

      unimplement:
	fprintf(stderr, "\033[1munimplememt\033[0m\n");

      fail:
	BUG();
	return -1;
}

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

/**
 * attach current to a container with pid
 */
int namespace_attach_to_container(pid_t pid)
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
