#ifndef CONTAINER_NAMESPACE_H
#define CONTAINER_NAMESPACE_H

#include <container.h>

/* unused */
#define NET_NAMESPACE "my_net_ns"

#ifdef OVERLAY_ROOTFS

#define LOWER_DIR "/"
#define UPPER_DIR "/tmp/upper"
#define WORK_DIR  "/tmp/work"

#endif /* OVERLAY_ROOTFS */

enum namespace {
	MNT,
	NET,
	PID,
	UTS,
	USER,
	CGROUP,
	IPC,
	PID_FOR_CHILDREN,
	/* tag for end */
	NULLNS,
};

enum filesystem_type {
	ROOTFS = 0,
	PROCFS,
	DEVFS,
	SYSFS,
	USRFS,
	ETCFS,
	VARFS,
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

int namespace_init_container_symlinks(const char *links[]);
int namespace_init_container_filesystem(const struct mount_args *args);
int namespace_attach_to_container(pid_t pid);

/* not sure */
int namespace_init_user_ns(pid_t pid, uid_t uid, gid_t gid);
/* has bug */
int namespace_preinit_net_ns();
/* has bug */
int namespace_attach_net_ns(pid_t pid);

#endif