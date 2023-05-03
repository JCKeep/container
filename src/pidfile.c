#include <pidfile.h>

int container_pid;
int container_exiting;

int container_run_pidfile(const char *path, pid_t pid)
{
	int pidfile = open(path, O_WRONLY | O_CREAT, 0644);
	if (write(pidfile, &pid, sizeof(pid_t)) < 0) {
		perror("container_run_pidfile");
		return -1;
	}
	close(pidfile);

	return 0;
}

int container_exit_pidfile(const char *path)
{
	int pid = -1;
	int pidfile = open(path, O_WRONLY | O_CREAT, 0644);
	if (write(pidfile, &pid, sizeof(pid_t)) < 0) {
		perror("container_exit_pidfile");
		return -1;
	}
	close(pidfile);

	return 0;
}

int container_get_pid()
{
	if (container_pid) {
		return container_pid;
	}

	int pid, pidfile;

	pidfile = open(PIDFILE, O_RDONLY);
	read(pidfile, &pid, sizeof(pid_t));

	container_pid = pid;
	close(pidfile);

	return pid;
}
