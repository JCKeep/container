#ifndef MNT_NAMESPACE_H
#define MNT_NAMESPACE_H

#include <container.h>
#include <c_namespace/ns_module.h>

struct mntns_ctx {
    union {
        struct {
            int (*init)(struct namespace_context *ctx);
            int (*attach)(struct namespace_context *ctx, int pid);
        };
        struct ns_module module;
    };

    char name[128];
    struct image_mnt *mnts;
};

int mntns_ctx_init(struct mntns_ctx *ns);

#endif