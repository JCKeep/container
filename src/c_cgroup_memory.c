#include <container.h>
#include <c_cgroup.h>
#include <config.h>

static int cgrp_mem_ctx_init(struct cgroup_context *_ctx)
{
    DIR *dir;
    char buf[1024];
    struct mem_cgrp_ctx *ctx = &_ctx->memory_ctx;

	dbg("cgrp_mem_ctx_init");

    snprintf(buf, sizeof(buf), CGROUP_SYS_CTRL "/memory/%s",
		 ctx->name);
    if (!dbg(dir = opendir(buf))) {
        mkdir(buf, 0755);
    } else {
        closedir(dir);
    }

    strcpy(ctx->name, MEMORY_CGROUP);
    /* default: 64M */
    ctx->limit_in_bytes = MEMORY_LIMIT;
    /* why? */
    ctx->tcp_limit_in_bytes = 9223372036854771712L;
    ctx->clone_children = 1;
    

    return 0;
}

static unsigned long parse_size(char *s)
{
    int len = strlen(s);
    unsigned long base = 1, num = 0;
    char *p = s, *q = s + len;

    
    while (!isalnum(*q)) {
        q--;
    }

    if (isalpha(*q)) {
        switch(*q) {
        case 'M':
        case 'm':
            base = 1024 * 1024;
            break;
        case 'K':
        case 'k':
            base = 1024;
            break;
        case 'G':
        case 'g':
            base = 1024 * 1024 * 1024;
            break;
        default:
            break;
        }
    }

    while (!isdigit(*p)) {
        p++;
    }

    num = strtoul(p, NULL, 10);
    return num * base;
}

static int cgrp_mem_ctx_parse(struct cgroup_context *_ctx,
			      struct config_parse_stat *stat)
{
	int __unused ret = 0;
	cJSON *cf = stat->json, *ccf = NULL;
	struct mem_cgrp_ctx *ctx = &_ctx->memory_ctx;

    dbg("cgrp_mem_ctx_parse");

    ccf = cJSON_GetObjectItem(cf, "memory_limit");
	if (ccf != NULL) {
		ctx->limit_in_bytes = parse_size(ccf->valuestring);
		dbg(ctx->limit_in_bytes);
	}

    ccf = cJSON_GetObjectItem(cf, "tcp_kmemory_limit");
	if (ccf != NULL) {
		ctx->tcp_limit_in_bytes = parse_size(ccf->valuestring);
		dbg(ctx->tcp_limit_in_bytes);
	}

    ccf = cJSON_GetObjectItem(cf, "clone_children");
	if (ccf != NULL) {
		ctx->clone_children = ccf->valueint;
		dbg(ctx->clone_children);
	}

    return 0;
}

static int __unused cgrp_mem_ctx_cgrpctl(struct cgroup_context *ctx,
					 unsigned long opt, void *data)
{
	return 0;
}

static int __unused cgrp_mem_ctx_attach(struct cgroup_context *_ctx)
{
    int ret = 0, fd;
	char buf[1024], opt[128];
	struct mem_cgrp_ctx *ctx = &_ctx->memory_ctx;

    memset(buf, 0, sizeof(buf));
	memset(opt, 0, sizeof(opt));

	dbg("cgrp_mem_ctx_attach");

    snprintf(buf, sizeof(buf), CGROUP_SYS_CTRL "/memory/%s/memory.limit_in_bytes",
		 ctx->name);
	fd = open(buf, O_WRONLY);
	if (fd < 0) {
		BUG();
		goto fail;
	}
	snprintf(opt, sizeof(opt), "%ld", ctx->limit_in_bytes);
	ret = write(fd, opt, strlen(opt) + 1);
	if (ret < 0) {
		BUG();
		goto fail;
	}
	close(fd);

    snprintf(buf, sizeof(buf), CGROUP_SYS_CTRL "/memory/%s/memory.kmem.tcp.limit_in_bytes",
		 ctx->name);
	fd = open(buf, O_WRONLY);
	if (fd < 0) {
		BUG();
		goto fail;
	}
	snprintf(opt, sizeof(opt), "%ld", ctx->tcp_limit_in_bytes);
	ret = write(fd, opt, strlen(opt) + 1);
	if (ret < 0) {
		BUG();
		goto fail;
	}
	close(fd);

    snprintf(buf, sizeof(buf), CGROUP_SYS_CTRL "/memory/%s/cgroup.clone_children",
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

	snprintf(buf, sizeof(buf), CGROUP_SYS_CTRL "/memory/%s/tasks", ctx->name);
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

int cgroup_mem_ctx_init(struct cpu_cgrp_ctx *ctx)
{
	ctx->init = cgrp_mem_ctx_init;
	ctx->parse = cgrp_mem_ctx_parse;
	ctx->attach = cgrp_mem_ctx_attach;
	ctx->cgrpctl = cgrp_mem_ctx_cgrpctl;

	return 0;
}
