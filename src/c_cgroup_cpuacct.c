#include <container.h>
#include <c_cgroup.h>
#include <config.h>

static int cgrp_cpuacct_ctx_init(struct cgroup_context *_ctx)
{
    DIR *dir;
    char buf[1024];
    struct cpuacct_cgrp_ctx *ctx = &_ctx->cpuacct_ctx;

	dbg("cgrp_cpuacct_ctx_init");

    snprintf(buf, sizeof(buf), CGROUP_SYS_CTRL "/cpuacct/%s",
		 ctx->name);
    if (!dbg(dir = opendir(buf))) {
        mkdir(buf, 0755);
    } else {
        closedir(dir);
    }

	strcpy(ctx->name, CPUACCT_CGROUP);
    ctx->enable = 1;

    return 0;
}

static int cgrp_cpuacct_ctx_parse(struct cgroup_context *_ctx,
			      struct config_parse_stat *stat)
{
    int __unused ret = 0;
	cJSON *cf = stat->json, *ccf = NULL;
	struct cpuacct_cgrp_ctx *ctx = &_ctx->cpuacct_ctx;

	dbg("cgrp_cpu_ctx_parse");

	ccf = cJSON_GetObjectItem(cf, "enable");
	if (ccf != NULL) {
		ctx->enable = ccf->valueint > 0 ? 1 : 0;
		dbg(ctx->enable);
	}

    return 0;
}

static int __unused cgrp_cpuacct_ctx_cgrpctl(struct cgroup_context *ctx,
					 unsigned long opt, void *data)
{
	return 0;
}

static int __unused cgrp_cpuacct_ctx_attach(struct cgroup_context *_ctx)
{
    int ret = 0, fd;
	char buf[1024], opt[128];
	struct cpuacct_cgrp_ctx *ctx = &_ctx->cpuacct_ctx;

	memset(buf, 0, sizeof(buf));
	memset(opt, 0, sizeof(opt));

	dbg("cgrp_cpuacct_ctx_attach");

    if (!ctx->enable) {
        return 0;
    }

	snprintf(buf, sizeof(buf), CGROUP_SYS_CTRL "/cpuacct/%s/tasks", ctx->name);
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

int cgroup_cpuacct_ctx_init(struct cpu_cgrp_ctx *ctx)
{
	ctx->init = cgrp_cpuacct_ctx_init;
	ctx->parse = cgrp_cpuacct_ctx_parse;
	ctx->attach = cgrp_cpuacct_ctx_attach;
	ctx->cgrpctl = cgrp_cpuacct_ctx_cgrpctl;

	return 0;
}