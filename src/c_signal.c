#include <container.h>
#include <c_signal.h>

sigset_t mask;
extern int container_exiting;

static void signal_handler(int sig)
{
	container_exiting = 1;
}

void container_init_signal()
{
	sigemptyset(&mask);
	sigaddset(&mask, SIGINT);
	sigaddset(&mask, SIGSTOP);

	sigprocmask(SIG_BLOCK, &mask, NULL);

	signal(SIGINT, signal_handler);
	signal(SIGSTOP, signal_handler);
}
