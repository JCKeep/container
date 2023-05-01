#include <c_namespace.h>

struct namespace_map {
	char *name;
	unsigned long flags;
};

#define DEFINE_NSMAP(_name, _flag) { .name = _name, .flags = _flag }

/** minimal container filesystems to mount */
const struct mount_args filesystems[] = {
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
#ifdef MINUSRFS
	[USRFS] = {
		   .name = "usrfs",
		    },
#else
	[USRFS] = {
		   .name = "usrfs",
		   .source = "/usr",
		   .target = ROOT "/usr",
		   .flags = MS_BIND,
		   .mode = 0555,
		    },
#endif
	[ETCFS] = {
		   .name = "etcfs",
		   .source = "/etc",
		   .target = ROOT "/etc",
		   .flags = MS_BIND,
		   .mode = 0555,
		    },
	/* tag for the end */
	[NULLFS] = { },
};

/**  */
const char *symlinks[] = {
	/* from    ->      to */
	"/usr/bin", "/bin",
	"/usr/lib", "/lib",
	"/usr/lib64", "/lib64",
	"/usr/sbin", "/sbin",
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
	const struct mount_args *fs = args, *end = &args[NULLFS];

	while (fs != end) {
		if (!fs->target || !fs->source) {
			goto unimplement;
		}

		if (mkdir(fs->target, fs->mode) < 0) {
			fprintf(stderr, "mkdir %s: %s\n", fs->target,
				strerror(errno));
			goto fail;
		}

		if (mount
		    (fs->source, fs->target, fs->filesystemtype, fs->flags,
		     fs->data) < 0) {
			fprintf(stderr, "mount %s: %s\n", fs->name,
				strerror(errno));
			goto fail;
		}

		fs++;
	}

	if (chroot(ROOT) < 0) {
		perror("chroot");
		goto fail;
	}

	if (mkdir("/root", 0755) < 0) {
		perror("mkdir /root");
		goto fail;
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
	char ns_path[512];
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
