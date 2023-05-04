#include <container.h>
#include <c_cgroup.h>
#include <config.h>

static int cgrp_cpu_ctx_init(struct cgroup_context *_ctx)
{
	DIR *dir;
	char buf[1024];
	struct cpu_cgrp_ctx *ctx = &_ctx->cpu_ctx;

	dbg("cgrp_cpu_ctx_init");

	snprintf(buf, sizeof(buf), CGROUP_SYS_CTRL "/cpu/%s", ctx->name);
	if (!dbg(dir = opendir(buf))) {
		mkdir(buf, 0755);
	} else {
		closedir(dir);
	}

	strcpy(ctx->name, CPU_CGROUP);
	ctx->cfs_period_us = 100000;
	ctx->cfs_quota_us = -1;
	ctx->rt_period_us = 1000000;
	ctx->rt_runtime_us = 950000;
	ctx->shares = 1024;
	ctx->clone_children = 1;
	ctx->sane_behavior = 0;

	return 0;
}

static int cgrp_cpu_ctx_parse(struct cgroup_context *_ctx,
			      struct config_parse_stat *stat)
{
	int __unused ret = 0;
	double dtmp;
	cJSON *cf = stat->json, *ccf = NULL;
	struct cpu_cgrp_ctx *ctx = &_ctx->cpu_ctx;

	if (!(stat->module & (CGRP_MODULE | CGRP_CPU_MODULE))) {
		perror("error parse handler");
		BUG();
		return -1;
	}

	ccf = cJSON_GetObjectItem(cf, "shares");
	if (ccf != NULL) {
		ctx->shares = ccf->valueint;
		dbg(ctx->shares);
	}

	ccf = cJSON_GetObjectItem(cf, "cfs_limit");
	if (ccf != NULL) {
		dtmp = ccf->valuedouble;

		if (dtmp > 1 || dtmp < 0) {
			dtmp = 1;
		}

		ctx->cfs_quota_us = (int)(dtmp * ctx->cfs_period_us);
		dbg(ctx->cfs_quota_us);
	}

	ccf = cJSON_GetObjectItem(cf, "rt_limit");
	if (ccf != NULL) {
		dtmp = ccf->valuedouble;

		if (dtmp > 1 || dtmp < 0) {
			dtmp = 1;
		}

		ctx->rt_runtime_us = (int)(dtmp * ctx->rt_runtime_us);
		dbg(ctx->rt_runtime_us);
	}

	ccf = cJSON_GetObjectItem(cf, "clone_children");
	if (ccf != NULL) {
		ctx->clone_children = ccf->valueint;
		dbg(ctx->clone_children);
	}

	ccf = cJSON_GetObjectItem(cf, "sane_behavior");
	if (ccf != NULL) {
		ctx->sane_behavior = ccf->valueint;
		dbg(ctx->sane_behavior);
	}

	return 0;
}

static int __unused cgrp_cpu_ctx_cgrpctl(struct cgroup_context *ctx,
					 unsigned long opt, void *data)
{
	return 0;
}

static int __unused cgrp_cpu_ctx_attach(struct cgroup_context *_ctx)
{
	int ret = 0, fd;
	char buf[1024], opt[128];
	struct cpu_cgrp_ctx *ctx = &_ctx->cpu_ctx;

	memset(buf, 0, sizeof(buf));
	memset(opt, 0, sizeof(opt));

	dbg("cgrp_cpu_ctx_attach");

	snprintf(buf, sizeof(buf), CGROUP_SYS_CTRL "/cpu/%s/cpu.shares",
		 ctx->name);
	fd = open(buf, O_WRONLY);
	if (fd < 0) {
		BUG();
		goto fail;
	}
	snprintf(opt, sizeof(opt), "%d", ctx->shares);
	ret = write(fd, opt, strlen(opt) + 1);
	if (ret < 0) {
		BUG();
		goto fail;
	}
	close(fd);

	snprintf(buf, sizeof(buf), CGROUP_SYS_CTRL "/cpu/%s/cpu.cfs_quota_us",
		 ctx->name);
	fd = open(buf, O_WRONLY);
	if (fd < 0) {
		BUG();
		goto fail;
	}
	snprintf(opt, sizeof(opt), "%d", ctx->cfs_quota_us);
	ret = write(fd, opt, strlen(opt) + 1);
	if (ret < 0) {
		BUG();
		goto fail;
	}
	close(fd);

	snprintf(buf, sizeof(buf), CGROUP_SYS_CTRL "/cpu/%s/cpu.rt_runtime_us",
		 ctx->name);
	fd = open(buf, O_WRONLY);
	if (fd < 0) {
		BUG();
		goto fail;
	}
	snprintf(opt, sizeof(opt), "%d", ctx->rt_runtime_us);
	ret = write(fd, opt, strlen(opt) + 1);
	if (ret < 0) {
		BUG();
		goto fail;
	}
	close(fd);

	snprintf(buf, sizeof(buf),
		 CGROUP_SYS_CTRL "/cpu/%s/cgroup.clone_children", ctx->name);
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

	snprintf(buf, sizeof(buf), CGROUP_SYS_CTRL "/cpu/%s/tasks", ctx->name);
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

int cgroup_cpu_ctx_init(struct cpu_cgrp_ctx *ctx)
{
	ctx->init = cgrp_cpu_ctx_init;
	ctx->parse = cgrp_cpu_ctx_parse;
	ctx->attach = cgrp_cpu_ctx_attach;
	ctx->cgrpctl = cgrp_cpu_ctx_cgrpctl;

	return 0;
}
