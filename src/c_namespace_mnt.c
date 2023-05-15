#include <container.h>
#include <c_namespace.h>

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
			BUG();
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
			BUG();
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
	return -1;
}

static int mnt_namespace_init(struct namespace_context *_ctx)
{
	int ret = 0;
	struct mntns_ctx *mntns = &_ctx->mntns;

	if (mntns->mnts == NULL) {
		ret = namespace_init_container_filesystem(filesystems, NULLFS);
	} else {
		ret = namespace_init_container_filesystem(mntns->mnts, NULLFS);
	}

	if (ret < 0) {
		BUG();
		return -1;
	}

	ret = namespace_init_container_symlinks(symlinks);
	if (ret < 0) {
		BUG();
		return -1;
	}

	return 0;
}

static int mnt_namespace_attach(struct namespace_context *_ctx, int pid)
{
	return 0;
}

int mntns_ctx_init(struct mntns_ctx *ns)
{
	ns->init = mnt_namespace_init;
	ns->attach = mnt_namespace_attach;

	strcpy(ns->name, "mntns");
	ns->mnts = NULL;

	return 0;
}
