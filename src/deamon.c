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

	if (container_run_pidfile(PIDFILE, getpid()) < 0) {
		return -1;
	}

	return 0;
}

#if 0
static int epfd, sock;

static int deamon_handler(struct epoll_event *ev)
{
	int n = 0, fd = ev->data.fd;
	char buf[256];

	n = read(fd, buf, sizeof(buf));
	if (n <= 0) {
		return n;
	}

	if (!strncmp("run", buf, sizeof("run") - 1)) {

	} else if (!strncmp("exec", buf, sizeof("exec") - 1)) {

	} else if (!strncmp("build", buf, sizeof("build") - 1)) {

	} else {
		fprintf(stderr, "No container_image_builder\n");
		return -1;
	}
}

int container_deamon(int argc, char *argv[])
{
	int ret = 0;
	struct sockaddr_un addr;
	struct epoll_event ev, events[32];

	sock = socket(AF_UNIX, SOCK_STREAM, 0);
	if (sock < 0) {
		fprintf(stderr, "Failed to create unix socket\n");
		return -1;
	}

	memset(&addr, 0, sizeof(addr));
	addr.sun_family = AF_UNIX;
	strncpy(addr.sun_path, CONTAINER_UNIX_SOCK,
		sizeof(CONTAINER_UNIX_SOCK) - 1);

	if (bind(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
		fprintf(stderr, "Failed to bind unix socket\n");
		return -1;
	}

	if (listen(sock, 32) < 0) {
		fprintf(stderr, "Failed to listen unix socket\n");
		return -1;
	}

	epfd = epoll_create(32);
	if (epfd < 0) {
		fprintf(stderr, "Faild to create epoll\n");
		BUG();
		return -1;
	}

	ev.data.u64 = 0;
	ev.data.fd = sock;
	ev.events = EPOLLIN | EPOLLET;
	ret = epoll_ctl(epfd, EPOLL_CTL_ADD, sock, &ev);
	if (ret < 0) {
		fprintf(stderr, "epoll_ctl: %s\n", strerror(errno));
		return -1;
	}

	for (;;) {
		int nevent = epoll_wait(epfd, events, 32, -1);
		if (!nevent) {
			continue;
		}

		for (int i = 0; i < nevent; i++) {
			if (events[i].data.fd == sock) {
				int client_fd = accept(sock, NULL, NULL);
				if (client_fd < 0) {
					fprintf(stderr,
						"Failed to accept connection.\n");
					continue;
				}

				ev.data.u64 = 0;
				ev.data.fd = client_fd;
				ev.events = EPOLLIN | EPOLLET;
				epoll_ctl(epfd, EPOLL_CTL_ADD, client_fd, &ev);
			} else {
				ret = deamon_handler(events + i);
				if (ret <= 0) {
					epoll_ctl(epfd, EPOLL_CTL_DEL,
						  events[i].data.fd, NULL);
					close(events[i].data.fd);
					continue;
				}
			}
		}
	}

}

#endif
