#ifndef CONTAINER_H
#define CONTAINER_H

#define _GNU_SOURCE
#include <stdio.h>
#include <sched.h>
#include <fcntl.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/mount.h>

#define CGROUP_SYS_CTRL "/sys/fs/cgroup"
#define CONTAINER_NAME  "demo-container"

#ifndef ROOT
#define ROOT "/tmp/" CONTAINER_NAME
#endif

#endif