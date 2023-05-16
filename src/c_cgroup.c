#include <c_cgroup.h>
#include <config.h>

static struct cgroup_context ctx;
struct cgroup_context *global_cgrp_ctx = &ctx;

struct cgrp_ctx_modules global_cgrp_ctx_modules[] = {
	{
	 .name = "cpu",
	 .module = CGRP_CPU_MODULE | CGRP_MODULE,
	 .offset = offsetof(struct cgroup_context, cpu_ctx),
	  },
	{
	 .name = "cpuset",
	 .module = CGRP_CPUSET_MODULE | CGRP_MODULE,
	 .offset = offsetof(struct cgroup_context, cpuset_ctx),
	  },
	{
	 .name = "memory",
	 .module = CGRP_MEM_MODULE | CGRP_MODULE,
	 .offset = offsetof(struct cgroup_context, memory_ctx),
	  },
	{
	 .name = "cpuacct",
	 .module = CGRP_CPUACCT_MODULE | CGRP_MODULE,
	 .offset = offsetof(struct cgroup_context, cpuacct_ctx),
	  },
	{ }
};

char *auto_cpu_cgroup[] = {
	[0] = AUTO_CGROUP,
	[NULLCGROUP] = NULL,
};

int cgroup_init_container_cgrp(pid_t pid)
{
#if 0
	int _pid;
	char spid[16], cpu_limit[16], mem_limit[16], cpuset_limit[16];

	switch ((_pid = fork())) {
	case -1:
		perror("fork");
		return -1;
	case 0:
		sprintf(spid, "%d", pid);
		auto_cpu_cgroup[CONTAINER_PID] = spid;
		sprintf(cpu_limit, "%d", CPU_LIMIT);
		auto_cpu_cgroup[CPU_CGROUP_LIMIT] = cpu_limit;
		sprintf(mem_limit, "%d", MEMORY_LIMIT);
		auto_cpu_cgroup[MEMORY_CGROUP_LIMIT] = mem_limit;
		sprintf(cpuset_limit, "%ld-%ld",
			cpuset_limit_from(CPUSET_LIMIT),
			cpuset_limit_to(CPUSET_LIMIT));
		auto_cpu_cgroup[CPUSET_CGROUP_LIMIT] = cpuset_limit;

		if (execvpe(auto_cpu_cgroup[0], auto_cpu_cgroup, environ) < 0) {
			perror("cgroup");
			return -1;
		}
	default:
		waitpid(_pid, NULL, 0);
		break;
	}
#endif
	return 0;
}

static int cgrp_ctx_init(struct cgroup_context *ctx)
{
	int ret = 0;

	if (ctx->cpu_ctx.init) {
		ret = ctx->cpu_ctx.init(ctx);
	}
	if (ret < 0) {
		perror("ctx->cpu_ctx.init(ctx)");
		BUG();
		return -1;
	}

	if (ctx->cpuacct_ctx.init) {
		ret = ctx->cpuacct_ctx.init(ctx);
	}
	if (ret < 0) {
		perror("ctx->cpuacct_ctx.init(ctx)");
		BUG();
		return -1;
	}

	if (ctx->cpuset_ctx.init) {
		ret = ctx->cpuset_ctx.init(ctx);
	}
	if (ret < 0) {
		perror("ctx->cpuset_ctx.init(ctx)");
		BUG();
		return -1;
	}

	if (ctx->memory_ctx.init) {
		ret = ctx->memory_ctx.init(ctx);
	}
	if (ret < 0) {
		perror("ctx->memory_ctx.init(ctx)");
		BUG();
		return -1;
	}

	return 0;
}

static int cgrp_ctx_parse(struct cgroup_context *ctx,
			  struct config_parse_stat *st)
{
	int ret = 0;
	char *base_module = ctx;
	struct cgroup_module *module;
	struct cgrp_ctx_modules *p = global_cgrp_ctx_modules;
	struct config_parse_stat nst;

	if (!(st->module & CGRP_MODULE)) {
		perror("error parse handler");
		goto fail;
	}

	while (p->name != NULL) {
		cJSON *cf = cJSON_GetObjectItem(st->json, p->name);
		if (cf == NULL) {
			printf("no %s cgroup config\n", p->name);
			goto next;
		}

		memcpy(&nst, st, sizeof(*st));
		nst.json = cf;
		nst.module = p->module;
		nst.name = p->name;

		dbg("cgrp_ctx_modules");
		module = base_module + p->offset;
		ret = module->parse(ctx, &nst);
		if (ret < 0) {
			perror("cgrp_ctx_modules");
			goto fail;
		}

	      next:
		p++;
	}

	return 0;

      fail:
	return -1;
}

static int cgrp_ctx_attach(struct cgroup_context *ctx)
{
	int __unused ret = 0;
	char __unused buf[1024], *base = ctx;
	struct cgroup_module *module;
	struct cgrp_ctx_modules *p = global_cgrp_ctx_modules;

	memset(buf, 0, sizeof(buf));

	while (p->name != NULL) {
		dbg("cgrp_ctx_attach");
		module = base + p->offset;
		ret = module->attach(ctx);

		if (ret < 0) {
			goto fail;
		}

	      next:
		p++;
	}

	return 0;

      fail:
	return -1;
}

int cgroup_ctx_init(struct cgroup_context *ctx)
{
	int ret;

	memset(ctx, 0, sizeof(struct cgroup_context));
	ctx->init = cgrp_ctx_init;
	ctx->parse = cgrp_ctx_parse;
	ctx->attach = cgrp_ctx_attach;

	/** 
	 * 如果新增 cgroup 模块，在下面进行初始化
	 */
	ret = cgroup_cpu_ctx_init(&ctx->cpu_ctx);
	if (ret < 0) {
		BUG();
		return -1;
	}

	ret = cgroup_cpuset_ctx_init(&ctx->cpuset_ctx);
	if (ret < 0) {
		BUG();
		return -1;
	}

	ret = cgroup_mem_ctx_init(&ctx->memory_ctx);
	if (ret < 0) {
		BUG();
		return -1;
	}

	ret = cgroup_cpuacct_ctx_init(&ctx->cpuacct_ctx);
	if (ret < 0) {
		BUG();
		return -1;
	}

	if (ctx->init) {
		ret = ctx->init(ctx);
	}
	if (ret < 0) {
		BUG();
		return -1;
	}

	return 0;
}
