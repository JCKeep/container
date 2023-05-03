#ifndef MEMORY_CGROUP_H
#define MEMORY_CGROUP_H

struct cgroup_context;

struct mem_cgrp_ctx {
    /* init cgroup ctx by default value */
    int (*init)(struct cgroup_context *ctx);
    /* modify cgroup ctx */
    int (*cgrpctl)(struct cgroup_context *ctx, unsigned long opt, void *data);
    /* attach to /sys/fs/cgroup/[subsystem]/[my_cgroup] */
    int (*attach)(struct cgroup_context *ctx);
};

#endif