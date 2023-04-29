#include <container.h>

#ifndef CONTAINER_NAMESPACE_H
#define CONTAINER_NAMESPACE_H

enum filesystem_type {
	ROOTFS = 0,
	PROCFS,
	DEVFS,
	SYSFS,
	USRFS,
	ETCFS,
	/* tag for end */
	NULLFS,
};

struct mount_args {
	const char *name;
	const char *source;
	const char *target;
	const char *filesystemtype;
	unsigned long flags;
	mode_t mode;
	void *data;
};

extern const char *symlinks[];
extern const struct mount_args filesystems[];

int init_container_symlinks(const char *links[]);
int init_container_filesystem(const struct mount_args *args);

#endif