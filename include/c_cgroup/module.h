#ifndef CGP_MODULE_H
#define CGP_MODULE_H

#include <container.h>
#include <c_cgroup.h>

/* cgroup module for parsing config */
struct cgroup_module {
    /* init cgroup ctx by default value */
    int (*init)(struct cgroup_context *ctx);
    /* parse config file */
    int (*parse)(struct cgroup_context *ctx, struct config_parse_stat *stat);
    /* modify cgroup ctx */
    int (*cgrpctl)(struct cgroup_context *ctx, unsigned long opt, void *data);
    /* attach to /sys/fs/cgroup/[subsystem]/[my_cgroup] */
    int (*attach)(struct cgroup_context *ctx);
};

#endif