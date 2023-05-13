#ifndef CONTAINER_NAMESPACE_H
#define CONTAINER_NAMESPACE_H

#include <container.h>
#include <c_namespace/ns_module.h>
#include <c_namespace/mnt.h>
#include <c_namespace/uts.h>
#include <c_namespace/pid.h>
#include <c_namespace/nscgroup.h>

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
	MNT = 0,
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

struct namespace_map {
	char *name;
	unsigned long flags;

	/* todo */
	int (*ns_handler)(struct namespace_map * ns);
};

struct namespace_context {
	struct mntns_ctx mntns;
	struct utsns_ctx utsns;

	int (*init)(struct namespace_context *ctx);
    int (*attach)(struct namespace_context *ctx, int pid);
};

extern struct namespace_context *ns_ctx;

struct ns_ctx_modules {
	char *name;

	/* data offset of cgroup context */
	uintptr_t offset;

	uint64_t module;

	int (*init)(struct namespace_context *ctx);

	void *ctx;
};

extern struct ns_ctx_modules global_ns_ctx_modules[];


// int namespace_init(struct namespace_context *ctx);
int namespace_ctx_init(struct namespace_context *ctx);
// int namespace_attach_to_container(struct namespace_context *ctx, pid_t pid);

/* not sure */
int namespace_init_user_ns(pid_t pid, uid_t uid, gid_t gid);
/* has bug */
int namespace_preinit_net_ns();
/* has bug */
int namespace_attach_net_ns(pid_t pid);

#endif