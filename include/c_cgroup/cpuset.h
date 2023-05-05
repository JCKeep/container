#ifndef CPUSET_CGROUP_H
#define CPUSET_CGROUP_H

#define CGRP_CPUSET_MODULE (0x1 << 3)

#include <container.h>
#include <c_cgroup.h>
#include <c_cgroup/module.h>

struct cgroup_context;
struct config_parse_stat;

struct cpuset_cgrp_ctx {
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
     * `cpuset.cpus`是用于控制该cgroup中的进程可以使用的CPU编号列表，其值是一个字符串，
     * 表示一个或多个CPU编号，各个CPU编号之间用逗号分隔。例如，如果将`cpuset.cpus`设置
     * 为"0,1,2"，则该cgroup中的进程只能在0、1、2号CPU上运行，其他CPU对该进程不可见。
     * 需要注意的是，`cpuset.cpus`中的CPU编号可以使用"-"(表示范围)和","(表示分隔)两个
     * 符号，以便表示更复杂的CPU集合。例如，如果将`cpuset.cpus`设置为"0-2,4,6-7"，则
     * 表示该cgroup中的进程可以在0、1、2、4、6、7号CPU上运行。
     */
    char cpus[64];

    /**
     * `cpuset.cpu_exclusive`是Linux内核中`cpuset`子系统中的一个参数，表示此cgroup中
     * 的所有任务是否可以独占其指定的CPU集合。当`cpuset.cpu_exclusive`设置为1时，表示
     * 只有此cgroup中的任务可以运行在指定的CPU集合上，其他cgroup中的任务不能运行在该CPU
     * 集合上。当`cpuset.cpu_exclusive`设置为0时，表示此cgroup中的任务可以与其他cgroup
     * 中的任务共享CPU集合。这个参数的主要作用是控制CPU的使用权限，可以用来限制一些任务
     * 的CPU使用，防止他们抢占其他任务的CPU资源。 
     */
    int cpu_exclusive;

    /**
     * `cpuset.sched_load_balance`是cgroup中的一个参数，用于控制进程是否允许在cgroup之
     * 间迁移。当`sched_load_balance`被设置为1时，进程允许在cgroup之间进行迁移，以实现
     * 负载均衡。如果`sched_load_balance`被设置为0，则禁止进程在cgroup之间进行迁移。这
     * 个参数主要用于控制调度器对cgroup内部进程的调度策略，对于不同的应用场景可以进行不同
     * 的设置。
     */
    int sched_load_balance;

    /* importance, must use */
    int cpu_mems;
};

int cgroup_cpuset_ctx_init(struct cpu_cgrp_ctx *ctx);

#endif