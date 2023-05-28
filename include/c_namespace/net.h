#ifndef NET_NAMESPACE_H
#define NET_NAMESPACE_H

#include <container.h>
#include <c_namespace/ns_module.h>

#define NETNS 0x10

/* nothing to do */
struct netns_ctx {
	union {
		struct {
			int (*init)(struct namespace_context *ctx);
			int (*attach)(struct namespace_context *ctx, int pid);
		};
		struct ns_module module;
	};

	char name[128];
};

int netns_ctx_init(struct netns_ctx *ns);

#endif