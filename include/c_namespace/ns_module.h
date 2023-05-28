#ifndef NS_MODULE_H
#define NS_MODULE_H

struct ns_module {
	int (*init)(struct namespace_context *ctx);
	int (*attach)(struct namespace_context *ctx, int pid);
};

#endif