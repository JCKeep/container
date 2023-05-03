#ifndef PIDFILE_H
#define PIDFILE_H

#include <container.h>

#ifndef PIDFILE
#define PIDFILE "./run/pidfile"
#endif

int container_get_pid();
int container_run_pidfile(const char *path, pid_t pid);
int container_exit_pidfile(const char *path);

#endif