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
#include <pidfile.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/mount.h>
#include <stdatomic.h>

#ifdef DEBUG_INFO
#include <dbg.h>
#else
#define dbg(x) ({x;})
#endif

#define CGROUP_SYS_CTRL "/sys/fs/cgroup"

#ifndef CONTAINER_NAME 
#define CONTAINER_NAME  "demo-container"
#endif

#ifndef ROOT
#define ROOT "/tmp/" CONTAINER_NAME
#endif

#ifndef HAVE_BUG
#define BUG() do { \
	fprintf(stderr, "BUG: failure at %s:%d/%s()!\n", __FILE__, __LINE__, __func__); \
	exit(EXIT_FAILURE); \
} while (0)
#endif


#define __deprecated __attribute__((deprecated))
#define __unused __attribute__((unused))
#define __alias(name) __attribute__((alias(name)))
#define __aligned(n) __attribute__((aligned(n)))
#define __constructor __attribute__((constructor))

int deamon();
int container_get_pid();
int container_init_environ(int flag);
int container_exit_pidfile(const char *path);
int container_run_pidfile(const char *path, pid_t pid);


#endif
