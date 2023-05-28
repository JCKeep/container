#ifndef CONTAINER_H
#define CONTAINER_H

#define _GNU_SOURCE
#include <cJSON.h>
#include <stdio.h>
#include <sched.h>
#include <fcntl.h>
#include <errno.h>
#include <ctype.h>
#include <stdint.h>
#include <signal.h>
#include <dirent.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>
#include <sys/un.h>
#include <pidfile.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/mman.h>
#include <sys/epoll.h>
#include <sys/types.h>
#include <sys/mount.h>
#include <stdatomic.h>
#include <sys/socket.h>

#ifndef RUST_BINDING_H
#define RUST_BINDING_H
#include <bindings/container_images.h>
#endif

#ifdef DEBUG_INFO
#include <dbg.h>
#else
#define dbg(x) ({ x; })
#endif

#define CGROUP_SYS_CTRL "/sys/fs/cgroup"

#ifndef CONTAINER_NAME
#define CONTAINER_NAME "demo-container"
#endif

#ifndef STACK_SIZE
/* container stack size: 1M */
#define STACK_SIZE (1 << 20)
#endif

#ifndef ROOT
#define ROOT "/tmp/" CONTAINER_NAME
#endif

#ifndef DATA
#define DATA "/root/D/kernel/demo-container/data"
#endif

#define CONTAINER_UNIX_SOCK "/root/D/kernel/demo-container/run/sock"
#define IMAGES_DIR "/root/D/kernel/demo-container/images"

#ifndef HAVE_BUG
#define BUG()                                                              \
	do {                                                               \
		fprintf(stderr, "BUG: failure at %s:%d/%s()!\n", __FILE__, \
			__LINE__, __func__);                               \
		exit(EXIT_FAILURE);                                        \
	} while (0)
#endif

#define __deprecated __attribute__((deprecated))
#define __unused __attribute__((unused))
#define __alias(name) __attribute__((alias(name)))
#define __aligned(n) __attribute__((aligned(n)))
#define __constructor __attribute__((constructor))
#define __bitwise __attribute__((bitwise))
#define __cleanup(func) __attribute__((cleanup(func)))

#define MAX_LAYER 32

struct container_image_builder {
	int argc;
	char **argv;
	int layers;
	char images[MAX_LAYER][256];
	char target_image[256];
};

int deamon();
int container_init_environ(int flag);
int container_image_analyze_layer(const char *image,
				  struct container_image_builder *c);
int container_image_prebuild(FILE *fp, struct container_image_builder *c,
			     const char *image);
int container_image_build_confirm(struct container_image_builder *c,
				  const char *image_name);
int container_build_image(int argc, char *argv[]);
int container_run_command(void *arg);

#endif
