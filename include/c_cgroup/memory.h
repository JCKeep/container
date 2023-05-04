#ifndef MEMORY_CGROUP_H
#define MEMORY_CGROUP_H

#define CGRP_MEM_MODULE (0x1 << 4)

#include <container.h>
#include <c_cgroup.h>
#include <c_cgroup/module.h>

struct cgroup_context;
struct config_parse_stat;

struct mem_cgrp_ctx {
    union {
        struct {
            /* init cgroup ctx by default value */
            int (*init)(struct cgroup_context *ctx);
            /* parse config file */
            int (*parse)(struct cgroup_context *ctx, struct config_parse_stat *stat);
            /* modify cgroup ctx */
            int (*cgrpctl)(struct cgroup_context *ctx, unsigned long opt, void *data);
            /* attach to /sys/fs/cgroup/[subsystem]/[my_cgroup] */
            int (*attach)(struct cgroup_context *ctx);
        };
        struct cgroup_module module;
    };
};

#endif