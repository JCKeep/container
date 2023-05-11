#include <c_namespace.h>

struct namespace_map {
	char *name;
	unsigned long flags;

	/* todo */
	int (*ns_handler)(struct namespace_map * ns);
};

#define DEFINE_NSMAP(_name, _flag) { .name = _name, .flags = _flag }

#ifdef OVERLAY_ROOTFS

static int overlayfs_pre_handler(struct image_mnt __unused * mnt)
{
	int __unused ret;
	DIR *dir = NULL;
	struct container_image_builder *cmd = mnt->private_data;
	char buf[2048], lower[1024], upper[128], work[128];
	char *dupper = UPPER_DIR;
	char *dwork = WORK_DIR;

	if (cmd != NULL) {
		goto images;
	}

      merge:
	if (!(dir = opendir(dupper))) {
		ret = mkdir(dupper, 0755);
	} else {
		dbg("dir exist");
		closedir(dir);
	}

	if (!(dir = opendir(dwork))) {
		ret = mkdir(dwork, 0755);
	} else {
		dbg("dir exist");
		closedir(dir);
	}

	dbg("overlay");
	return 0;

      images:
	switch (cmd->layers) {
	case 1:
		snprintf(lower, sizeof(lower), "%s/diff", cmd->images[0]);
		break;
	case 2:
		snprintf(lower, sizeof(lower), "%s/diff:%s/diff",
			 cmd->images[0], cmd->images[1]);
		break;
	case 3:
		snprintf(lower, sizeof(lower), "%s/diff:%s/diff:%s/diff",
			 cmd->images[0], cmd->images[1], cmd->images[2]);
		break;
	default:
		snprintf(lower, sizeof(lower), "%s/diff",
			 "/root/D/kernel/demo-container/images/ubuntu_latest");
		break;
	}

	if (cmd->target_image[0] == '\0') {
		snprintf(upper, sizeof(upper), "%s/upper", cmd->images[0]);
		snprintf(work, sizeof(work), "%s/work", cmd->images[0]);
	} else {
		mkdir(cmd->target_image, 0755);
		snprintf(upper, sizeof(upper), "%s/diff", cmd->target_image);
		snprintf(work, sizeof(work), "%s/work", cmd->target_image);
	}
	snprintf(buf, sizeof(buf), "lowerdir=%s,upperdir=%s,workdir=%s", lower,
		 upper, work);

	mnt->data = strdup(buf);
	dupper = upper;
	dwork = work;

	goto merge;
}

static int overlayfs_post_handler(struct image_mnt __unused * mnt)
{
	if (mnt->private_data) {
		free(mnt->data);
	}

	return 0;
}
#endif

/** minimal container filesystems to mount */
static struct image_mnt ___filesystems[] = {
#ifdef OVERLAY_ROOTFS
	[ROOTFS] = {
		    .name = "rootfs",
		    .source = "overlay",
		    .target = ROOT,
		    .filesystemtype = "overlay",
		    .flags = MS_MGC_VAL,
		    .mode = 0755,
		    .data =
#ifndef IMAGE_SUPPORT
		    "lowerdir=" LOWER_DIR ",upperdir=" UPPER_DIR ",workdir="
		    WORK_DIR,
#else
		    "lowerdir=/tmp/lower1:" LOWER_DIR ",upperdir=" UPPER_DIR
		    ",workdir=" WORK_DIR,
#endif
		    .pre_handler = overlayfs_pre_handler,
		    .post_handler = overlayfs_post_handler,
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
const static char *___symlinks[] = {
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

char **symlinks = ___symlinks;
struct image_mnt *filesystems = ___filesystems;

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
int namespace_init_container_filesystem(const struct image_mnt *args, int len)
{
	int ret = 0;
	const struct image_mnt *mnt = args, *end = &args[len];

	while (mnt != end) {
		if (!mnt->target || !mnt->source) {
			goto next;
		}

		if (mkdir(mnt->target, mnt->mode) < 0) {
#ifndef OVERLAY_ROOTFS
			fprintf(stderr, "mkdir %s: %s\n", mnt->target,
				strerror(errno));
			goto fail;
#endif
		}

		if (mnt->pre_handler) {
			ret = mnt->pre_handler(mnt);
		}

		if (ret < 0) {
			goto fail;
		}

		if (mount
		    (mnt->source, mnt->target, mnt->filesystemtype, mnt->flags,
		     mnt->data) < 0) {
			fprintf(stderr, "mount %s: %s\n", mnt->name,
				strerror(errno));
			goto fail;
		}

		if (mnt->post_handler) {
			ret = mnt->post_handler(mnt);
		}

		if (ret < 0) {
			goto fail;
		}

	      next:
		mnt++;
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
