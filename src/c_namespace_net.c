#include <container.h>
#include <c_namespace.h>

static int net_namespace_init(struct namespace_context *_ctx)
{
	int pid, root_netns, pipefd[2];
	struct netns_ctx *ctx = &_ctx->netns;

	char *args[] = {
		"/bin/ip",
		"link",
		"set",
		"lo",
		"up",
		NULL,
	};

	switch ((pid = fork())) {
	case -1:
		BUG();
		return -1;
	case 0:
		if (execv(args[0], args) < 0) {
			perror("execv");
			BUG();
			return -1;
		}
	default:
		waitpid(pid, NULL, 0);
		break;
	}

	// // 创建虚拟网络设备对
	// if (system("ip link add veth0 type veth peer name veth1") == -1) {
	//     perror("system");
	//     return -1;
	// }

	// // 将 veth1 连接到主机网络
	// if (system("ip link set veth1 up") == -1) {
	//     perror("system");
	//     return -1;
	// }

	// // 在新命名空间中配置网络参数
	// if (system("ip addr add 192.168.0.1/24 dev veth0") == -1) {
	//     perror("system");
	//     return -1;
	// }

	return 0;
}

static int net_namespace_attach(struct namespace_context *_ctx, int pid)
{
	return 0;
}

int netns_ctx_init(struct netns_ctx *ns)
{
	ns->init = net_namespace_init;
	ns->attach = net_namespace_attach;

	strcpy(ns->name, "netns");

	return 0;
}
