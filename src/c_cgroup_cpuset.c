#include <container.h>
#include <c_cgroup.h>
#include <config.h>

static int cgrp_cpuset_ctx_init(struct cgroup_context *_ctx) 
{
	DIR *dir;
    char buf[1024];
    struct cpuset_cgrp_ctx *ctx = &_ctx->cpuset_ctx;

	dbg("cgrp_cpuset_ctx_init");

    snprintf(buf, sizeof(buf), CGROUP_SYS_CTRL "/cpuset/%s",
		 ctx->name);
    if (!dbg(dir = opendir(buf))) {
        mkdir(buf, 0755);
    } else {
        closedir(dir);
    }

	strcpy(ctx->name, CPUSET_CGROUP);
    ctx->clone_children = 1;
    ctx->sched_load_balance = 1;
    ctx->cpu_exclusive = 1;
    strcpy(ctx->cpus, "0-0");

    return 0;
}

static int cgrp_cpuset_ctx_parse(struct cgroup_context *_ctx,
			      struct config_parse_stat *stat)
{
    int __unused ret = 0;
    cJSON *cf = stat->json, *ccf = NULL;
	struct cpuset_cgrp_ctx *ctx = &_ctx->cpuset_ctx;

    dbg("cgrp_cpuset_ctx_parse");

    ccf = cJSON_GetObjectItem(cf, "clone_children");
    if (ccf != NULL) {
        ctx->clone_children = ccf->valueint;
        dbg(ctx->clone_children);
    }

    ccf = cJSON_GetObjectItem(cf, "cpus");
    if (ccf != NULL) {
        strcpy(ctx->cpus, ccf->valuestring);
        dbg(ctx->cpus);
    }

    ccf = cJSON_GetObjectItem(cf, "load_balance");
    if (ccf != NULL) {
        ctx->sched_load_balance = ccf->valueint;
        dbg(ctx->sched_load_balance);
    }

    ccf = cJSON_GetObjectItem(cf, "cpu_exclusive");
    if (ccf != NULL) {
        ctx->cpu_exclusive = ccf->valueint;
        dbg(ctx->cpu_exclusive);
    }

    return 0;
}

static int __unused cgrp_cpuset_ctx_cgrpctl(struct cgroup_context *ctx,
					 unsigned long opt, void *data)
{
	return 0;
}

static int __unused cgrp_cpuset_ctx_attach(struct cgroup_context *_ctx)
{
    int ret = 0, fd;
	char buf[1024], opt[128];
	struct cpuset_cgrp_ctx *ctx = &_ctx->cpuset_ctx;

	memset(buf, 0, sizeof(buf));
	memset(opt, 0, sizeof(opt));

    dbg("cgrp_cpuset_ctx_attach");

    snprintf(buf, sizeof(buf), CGROUP_SYS_CTRL "/cpuset/%s/cpuset.cpus",
		 ctx->name);
	fd = open(buf, O_WRONLY);
	if (fd < 0) {
		BUG();
		goto fail;
	}
	ret = write(fd, ctx->cpus, strlen(ctx->cpus) + 1);
	if (ret < 0) {
		BUG();
		goto fail;
	}
	close(fd);

    snprintf(buf, sizeof(buf), CGROUP_SYS_CTRL "/cpuset/%s/cpuset.cpu_exclusive",
		 ctx->name);
	fd = open(buf, O_WRONLY);
	if (fd < 0) {
		BUG();
		goto fail;
	}
	snprintf(opt, sizeof(opt), "%d", ctx->cpu_exclusive);
	ret = write(fd, opt, strlen(opt) + 1);
	if (ret < 0) {
		BUG();
		goto fail;
	}
	close(fd);

    snprintf(buf, sizeof(buf), CGROUP_SYS_CTRL "/cpuset/%s/cpuset.sched_load_balance",
		 ctx->name);
	fd = open(buf, O_WRONLY);
	if (fd < 0) {
		BUG();
		goto fail;
	}
	snprintf(opt, sizeof(opt), "%d", ctx->sched_load_balance);
	ret = write(fd, opt, strlen(opt) + 1);
	if (ret < 0) {
		BUG();
		goto fail;
	}
	close(fd);

    snprintf(buf, sizeof(buf), CGROUP_SYS_CTRL "/cpuset/%s/cgroup.clone_children",
		 ctx->name);
	fd = open(buf, O_WRONLY);
	if (fd < 0) {
		BUG();
		goto fail;
	}
	snprintf(opt, sizeof(opt), "%d", ctx->clone_children);
	ret = write(fd, opt, strlen(opt) + 1);
	if (ret < 0) {
		BUG();
		goto fail;
	}
	close(fd);

    snprintf(buf, sizeof(buf), CGROUP_SYS_CTRL "/cpuset/%s/tasks", ctx->name);
	fd = open(buf, O_WRONLY);
	if (fd < 0) {
		BUG();
		goto fail;
	}
	snprintf(opt, sizeof(opt), "%d", getpid());
	ret = write(fd, opt, strlen(opt) + 1);
	if (ret < 0) {
		BUG();
		goto fail;
	}
	close(fd);

    return 0;

fail:
    return -1;
}

int cgroup_cpuset_ctx_init(struct cpu_cgrp_ctx *ctx)
{
	ctx->init = cgrp_cpuset_ctx_init;
	ctx->parse = cgrp_cpuset_ctx_parse;
	ctx->attach = cgrp_cpuset_ctx_attach;
	ctx->cgrpctl = cgrp_cpuset_ctx_cgrpctl;

	return 0;
}

