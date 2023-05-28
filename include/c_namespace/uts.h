#ifndef UTS_NAMESPACE_H
#define UTS_NAMESPACE_H

#include <container.h>
#include <c_namespace/ns_module.h>

#define UTSNS 0x02

struct utsns_ctx {
	union {
		struct {
			int (*init)(struct namespace_context *ctx);
			int (*attach)(struct namespace_context *ctx, int pid);
		};
		struct ns_module module;
	};

	char name[128];

	char container_name[256];
};

int utsns_ctx_init(struct utsns_ctx *ns);

#endif