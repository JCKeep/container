#ifndef CPU_CGROUP_H
#define CPU_CGROUP_H

#include <container.h>
#include <c_cgroup.h>
#include <c_cgroup/module.h>

#define CGRP_CPU_MODULE (0x1 << 1)
#define CFS_PERIOD_US 100000
#define DEFAULT_SHARES 1024

/**
 * cpu cgroup 配置
*/
struct cpu_cgrp_ctx {
    union {
        struct {
            /* init cgroup ctx by default value */
            int (*init)(struct cgroup_context *ctx);
            /* parse config file */
            int (*parse)(struct cgroup_context *ctx, struct config_parse_stat *stat);
            /* unused interface */
            int (*cgrpctl)(struct cgroup_context *ctx, unsigned long opt, void *data);
            /* attach to /sys/fs/cgroup/[subsystem]/[my_cgroup] */
            int (*attach)(struct cgroup_context *ctx);
        };
        struct cgroup_module module;
    };
    
    char name[128];

    /**
     * `cgroup.clone_children` 是 cgroup v2 控制组中的一个标志，用于控制子进程的行为。
     * 当设置 `cgroup.clone_children` 为 1 时，表示该 cgroup 中的进程 fork 出的子进程
     * 会自动加入该 cgroup。同时，子进程可以创建自己的子进程，这些孙子进程也会自动加入该
     *  cgroup。这种自动继承行为，也被称为 "递归子系统"（recursive subsystem）。当设置 
     * `cgroup.clone_children` 为 0 时，则表示子进程不会自动加入该 cgroup，而是继承该
     *  cgroup 的限制。需要注意的是，`cgroup.clone_children` 只在 cgroup v2 中生效，
     * 而 cgroup v1 中则默认为启用递归子系统。
    */
    int clone_children;

    /**
     * `cgroup.sane_behavior`是一个cgroup v1的内核参数，用于控制cgroup在OOM（Out of Memory）
     * 发生时的行为。当`sane_behavior`设置为1时，cgroup在OOM发生时会尝试重新分配一部分内存以
     * 防止系统崩溃。如果`sane_behavior`设置为0，则cgroup会将OOM事件传递给子进程，由子进程自
     * 己处理OOM。需要注意的是，`cgroup.sane_behavior`只适用于cgroup v1，cgroup v2已经取消
     * 了该参数。*/
    int sane_behavior;

    /**
     * cfs_quota_us / cfs_period_us 为进程最大cpu使用比，同一cpu cgroup中所有进程共享
    */
    int cfs_period_us;
    int cfs_quota_us;

    /**
     * rt_runtime_us / rt_period_us: rt_throttling 最大cpu运行比
    */
    int rt_period_us;
    int rt_runtime_us;

    /**
     * `cpu.shares` 是 Linux Cgroup 中用于设置 CPU 资源分配比例的参数。它是一个权重值，可以理解
     * 为一个相对权重，表示该 Cgroup 相对于其他同级别的 Cgroup 分配 CPU 时间片的相对比例。例如，
     * 如果一个 Cgroup 的 `cpu.shares` 设置为 100，另一个 Cgroup 的 `cpu.shares` 设置为 50，
     * 则前者将获得比后者多两倍的 CPU 时间片。`cpu.shares` 参数是针对单个 CPU 的。例如，如果系统
     * 有 4 个 CPU，则 `cpu.shares` 设置为 200 的 Cgroup 将获得系统 CPU 时间的 50%。如果有多个
     * 进程在同一 Cgroup 中，则它们将均分该 Cgroup 分配的 CPU 时间。需要注意的是，`cpu.shares` 
     * 只是相对权重，并不能保证精确的 CPU 时间分配，因为还有其他因素可能会影响 CPU 时间的分配，如
     *  CPU 的实际利用率等。
    */
    int shares;
};

int cgroup_cpu_ctx_init(struct cpu_cgrp_ctx *ctx);

#endif