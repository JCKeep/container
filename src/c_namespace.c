#include <c_namespace.h>

/* minimal container filesystems to mount */
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
	[NULLFS] = { },
};

const char *symlinks[] = {
	"/usr/bin", "/bin",
	"/usr/lib", "/lib",
	"/usr/lib64", "/lib64",
	"/usr/sbin", "/sbin",
	NULL,
};

int init_container_symlinks(const char *links[])
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
int init_container_filesystem(const struct mount_args *args)
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
	fprintf(stderr, "unimplememt\n");

      fail:
	return -1;
}
