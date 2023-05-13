#ifndef CGROUP_NAMESPACE_H
#define CGROUP_NAMESPACE_H

#include <container.h>
#include <c_namespace/ns_module.h>

#define CGROUPNS 0x04

/* nothing to do */
struct cgroupns_ctx {
    union {
        struct {
            int (*init)(struct namespace_context *ctx);
            int (*attach)(struct namespace_context *ctx, int pid);
        };
        struct ns_module module;
    };

    char name[128];
};

#endif