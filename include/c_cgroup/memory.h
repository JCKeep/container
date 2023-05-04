#ifndef MEMORY_CGROUP_H
#define MEMORY_CGROUP_H

#define CGRP_MEM_MODULE (0x1 << 4)

#include <container.h>
#include <c_cgroup.h>
#include <c_cgroup/module.h>


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

    char name[128];

    int clone_children;

    /**
     * `memory.limit_in_bytes` 是 Linux 系统中的内核参数，它设置了进程可以使用的最大
     * 内存限制。这个限制是以字节为单位指定的，应用于进程的驻留集大小 (RSS) 之和。
     */
    unsigned long limit_in_bytes;

    /**
     * `memory.kmem.tcp.limit_in_bytes` 是 Linux 系统中的一个内核参数，用于设置 TCP 
     * 协议栈可以使用的内存上限。TCP 协议栈是 Linux 系统中用于处理网络连接的核心组件之
     * 一，它需要使用一定量的内存来维护连接状态和传输数据。这个参数的单位是字节，它限制
     * 了 TCP 协议栈可以使用的内核内存上限。如果设置得过低，可能会导致系统出现网络问题，
     * 如果设置得过高，可能会浪费系统资源。在网络高负载的情况下，调整这个参数可能会对系
     * 统性能产生影响。
     */
    unsigned long tcp_limit_in_bytes;
};

int cgroup_mem_ctx_init(struct cpu_cgrp_ctx *ctx);

#endif