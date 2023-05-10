#ifndef CONTAINER_H
#define CONTAINER_H

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
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/mman.h>
#include <sys/epoll.h>
#include <sys/types.h>
#include <sys/mount.h>
#include <stdatomic.h>
#include <sys/socket.h>

#ifndef CGROUP_v1
#define CGROUP_SYS_CTRL "/sys/fs/cgroup/unified"
#else
#define CGROUP_SYS_CTRL "/sys/fs/cgroup"
#endif

#ifndef CONTAINER_NAME 
#define CONTAINER_NAME  "demo-container"
#endif

#ifndef ROOT
#define ROOT "/tmp/" CONTAINER_NAME
#endif

#ifndef DATA
#define DATA "/root/D/kernel/demo-container/data"
#endif

#define CONTAINER_UNIX_SOCK "/root/D/kernel/demo-container/run/sock"
#define IMAGES_DIR "/root/D/kernel/demo-container/images"

#endif