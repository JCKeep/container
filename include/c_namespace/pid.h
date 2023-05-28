#ifndef PID_NAMESPACE_H
#define PID_NAMESPACE_H

#include <container.h>
#include <c_namespace/ns_module.h>

#define PIDNS 0x04

/* nothing to do */
struct pidns_ctx {
	union {
		struct {
			int (*init)(struct namespace_context *ctx);
			int (*attach)(struct namespace_context *ctx, int pid);
		};
		struct ns_module module;
	};

	char name[128];
};

#endif