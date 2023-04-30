#include <container.h>

int container_run_deamon;

int deamon()
{
	int fd;

	if (!container_run_deamon) {
		return 0;
	}

	switch (fork()) {
	case -1:
		fprintf(stderr, "fork(): %s\n", strerror(errno));
		return -1;
	case 0:
		break;
	default:
		exit(EXIT_SUCCESS);
	}

	if (setsid() < 0) {
		fprintf(stderr, "setsid(): %s\n", strerror(errno));
		return -1;
	}

	umask(0);

	fd = open("/dev/null", O_RDWR);
	if (fd < 0) {
		fprintf(stderr, "open(): %s\n", strerror(errno));
		return -1;
	}

	if (dup2(fd, STDIN_FILENO) < 0) {
		fprintf(stderr, "dup2(STDIN): %s\n", strerror(errno));
		return -1;
	}

	if (dup2(fd, STDOUT_FILENO) < 0) {
		fprintf(stderr, "dup2(STDOUT): %s\n", strerror(errno));
		return -1;
	}

	printf("write pid\n");
	if (container_run_pidfile(PIDFILE, getpid()) < 0) {
		return -1;
	}

	return 0;
}
