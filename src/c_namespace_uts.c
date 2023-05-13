#include <container.h>
#include <c_namespace.h>

static int uts_namespace_init(struct namespace_context *_ctx)
{
	struct utsns_ctx *ctx = &_ctx->utsns;

	/** safety in uts_namespace, we set our own container hostname */
	if (sethostname(ctx->container_name, strlen(ctx->container_name)) < 0) {
		perror("sethostname");
		BUG();
		return -1;
	}

	return 0;
}

static int uts_namespace_attach(struct namespace_context *_ctx, int pid)
{
	return 0;
}

int utsns_ctx_init(struct utsns_ctx *ns)
{
	ns->init = uts_namespace_init;
	ns->attach = uts_namespace_attach;

	strcpy(ns->name, "utsns");
	strcpy(ns->container_name, CONTAINER_NAME);

	return 0;
}
