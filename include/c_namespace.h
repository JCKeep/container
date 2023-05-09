#ifndef CONTAINER_NAMESPACE_H
#define CONTAINER_NAMESPACE_H

#include <container.h>

/* unused */
#define NET_NAMESPACE "my_net_ns"

#ifdef OVERLAY_ROOTFS

#ifndef IMAGE_SUPPORT
#define LOWER_DIR "/"
#else
#define LOWER_DIR "/root/D/kernel/demo-container/images/ubuntu_latest/diff"
#endif
#define MERGE_DIR  "/root/D/kernel/demo-container/run/merge"
#define UPPER_DIR "/root/D/kernel/demo-container/run/upper"
#define WORK_DIR  "/root/D/kernel/demo-container/run/work"

#endif /* OVERLAY_ROOTFS */

enum snamespace {
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
	MOUNT_1,
	MOUNT_2,
	MOUNT_3,
	/* tag for end */
	NULLFS,
};

struct mount_args;
typedef int (*mount_handler)(struct mount_args *fs);

struct mount_args {
	const char *name;
	const char *source;
	const char *target;
	const char *filesystemtype;
	unsigned long flags;
	mode_t mode;
	void *data;

	/* use by mount_handler */
	void *private_data;
	mount_handler pre_handler;
	mount_handler post_handler;
};

extern const char *symlinks[];
extern const struct mount_args filesystems[];

int namespace_init_container_symlinks(const char *links[]);
int namespace_init_container_filesystem(const struct mount_args *args, int len);
int namespace_attach_to_container(pid_t pid);

/* not sure */
int namespace_init_user_ns(pid_t pid, uid_t uid, gid_t gid);
/* has bug */
int namespace_preinit_net_ns();
/* has bug */
int namespace_attach_net_ns(pid_t pid);

#endif