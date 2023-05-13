#ifndef MNT_NAMESPACE_H
#define MNT_NAMESPACE_H

#include <container.h>
#include <c_namespace/ns_module.h>

#define MNTNS 0x01

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


struct image_mnt;
typedef int (*mount_handler)(struct image_mnt *fs);

struct image_mnt {
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


extern char **symlinks;
extern struct image_mnt *filesystems;

struct mntns_ctx {
    union {
        struct {
            int (*init)(struct namespace_context *ctx);
            int (*attach)(struct namespace_context *ctx, int pid);
        };
        struct ns_module module;
    };

    char name[128];

    /* if NULL, use gloabl */
    struct image_mnt *mnts;
};



int mntns_ctx_init(struct mntns_ctx *ns);

#endif