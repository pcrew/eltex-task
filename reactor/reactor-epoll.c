
#include <fcntl.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>

#include <sys/epoll.h>
#include <sys/ioctl.h>

#include <pthread.h>

#include <sys/timerfd.h>
#include <sys/signalfd.h>

#include "reactor/reactor-api.h"
#include "utils/logging.h"
#include "thread-info.h"
#include "compiler.h"
#include "channel.h"

#include "handler.h"

static int epollfd;
static pthread_mutex_t mutex;

struct channel *ch;

#define MUTEX_LOCK	pthread_mutex_lock(&mutex)
#define MUTEX_UNLOCK	pthread_mutex_unlock(&mutex)

__attribute__((constructor)) 
static void reactor_mutex_init(void)
{
	int ret;

	ret = pthread_mutex_init(&mutex, NULL);
	if (ret) {
		log_err_msg("%s() - pthread_mutex_init() failed: %s\n", __func__, strerror(errno));
		exit(1);
	}

	log_dbg_msg("%s() - ((constructor)) - ok\n", __func__);
}

static void reactor_epoll__init(u32 nevents)
{
	epollfd = epoll_create(nevents);
	if (-1 == epollfd) {
		log_ftl_msg("%s() - epoll_create('%d'): %s\n", __func__, nevents, strerror(errno));
		exit(1);
	}
}

static void reactor_epoll__fini(void)
{
	int ret;

	ret = close(epollfd);

	if (-1 == ret) {
		log_sys_msg("%s() - close('epollfd: %d') %s\n", __func__, epollfd, strerror(errno));
		exit(1);
	}
}

static int reactor_epoll__add(int fd, void *dptr)
{
	MUTEX_LOCK;

	struct epoll_event ev = {0};
	   int flags;
	   int ret;

	ev.data.fd = fd;
	ev.data.ptr = dptr;

	ret = epoll_ctl(epollfd, EPOLL_CTL_ADD, fd, &ev);
	if (-1 == ret) {
		log_sys_msg("%s() - epoll_ctl('%d', 'ADD', '%d') failed\n", __func__, epollfd, fd);
		return 1;
	}

	flags = fcntl(fd, F_GETFL);
	if (-1 == flags) {
		log_sys_msg("%s() - fcntl(%d, 'GETFL') failed\n", __func__, fd);
		return 1;
	}

	flags |= O_NONBLOCK;

	ret = fcntl(fd, F_SETFL, flags);
	if (-1 == ret) {
		log_sys_msg("%s() - fcntl(%d, 'SETFL') failed\n", __func__, fd);
		return 1;
	}

	MUTEX_UNLOCK;
	return 0;
}

static int reactor_epoll__del(int fd)
{
	MUTEX_LOCK;

	struct epoll_event ev = {0};
	   int ret;

	ret = epoll_ctl(epollfd, EPOLL_CTL_DEL, fd, &ev);
	if (-1 == ret) {
		log_sys_msg("%s() - epoll_ctl('%d', 'DEL', '%d') failed\n", __func__, epollfd, fd);
		return 1;
	}

	MUTEX_UNLOCK;
	return 0;
}

static int reactor_epoll__mod(int fd, void *dptr, u32 events)
{
	MUTEX_LOCK;

	struct epoll_event ev = {0};
	   int ret;

	ev.data.ptr = dptr;
	ev.events = events;

	ret = epoll_ctl(epollfd, EPOLL_CTL_MOD, fd, &ev);
	if (-1 == ret) {
		log_sys_msg("%s() - epoll_ctl('%d', MOD, '%d') failed\n", __func__, epollfd, fd);
		return 1;
	}
	
	MUTEX_UNLOCK;
	return 0;
}

#define MAX_EVENTS 32
static struct epoll_event events[MAX_EVENTS];

static int reactor_epoll__wait(void)
{
static	struct signalfd_siginfo si = {0};
static     u64 tick_cnt = 0;
	   int nfds;
	   int ret;
	   int fd;
	   int i;

	nfds = epoll_wait(epollfd, events, MAX_EVENTS, -1); /* infinit timeout */
	if (unlikely(-1 == nfds)) {
		log_err_msg("%s() - epoll_wait() - failed\n", __func__);
		exit(1);
	}

	for (i = 0; i < nfds; i++) {

		ch = events[i].data.ptr;
		fd = ch->fd;

		switch(ch->type) {

			case CH_TICK:

				ret = read(fd, &tick_cnt, 8);
				if (ret != 8) {
					log_sys_msg("%s() - read event error\n", __func__);
					exit(1);
				}

				ret = event_handler(ch);
				if (ret) {
					log_err_msg("%s() - event_handler() - failed\n", __func__);
					exit(1);
				}
				break;

			case CH_SIGN:

				ret = read(fd, &si, sizeof(struct signalfd_siginfo));
				if (ret != sizeof(struct signalfd_siginfo)) {
					log_sys_msg("%s() - can't read signal event\n", __func__);
					exit(1);
				}

				ret = signal_handler(ch);
				if (ret) {
					log_err_msg("%s() - signal_handler() - failed\n", __func__);
					exit(1);
				}

				break;

			default:
				break;
		}
		
	}

	return 0;
}

static int reactor_epoll__new_sig(int sig)
{
	int fd;
	sigset_t mask;

	sigemptyset(&mask);
	sigaddset(&mask, sig);
	sigprocmask(SIG_BLOCK, &mask, NULL);
	
	fd = signalfd(-1, &mask, SFD_CLOEXEC);
	if (unlikely(-1 == fd)) {
		log_sys_msg("%s() - signalfd() failed: %s\n", __func__, strerror(errno));
		return -1;
	}

	return fd;
}

static int reactor_epoll__new_timerfd(void)
{
	int fd;

	fd = timerfd_create(CLOCK_REALTIME, TFD_CLOEXEC);
	if (unlikely(-1 == fd)) {
		log_sys_msg("%s() - timerfd_create(): %s\n", __func__, strerror(errno));
		return -1;
	}

	return fd;
}

static int reactor_epoll__arm_timerfd(int fd, int interval_msec)
{
	struct itimerspec its = {0};
	   int ret;

	its.it_value.tv_sec = interval_msec / 1000;
	its.it_value.tv_nsec = (interval_msec % 1000) * 1000000;

	ret = timerfd_settime(fd, 0, &its, NULL);
	if (unlikely(-1 == ret)) {
		log_sys_msg("%s() - timerfd_settime('%d'): %s\n", __func__, fd, strerror(errno));
		return 1;
	}

	return 0;
}

static int reactor_epoll__disarm_timerfd(int fd)
{
	struct itimerspec its = {0};
	   int ret;

	ret = timerfd_settime(fd, 0, &its, NULL);
	if (unlikely(-1 == ret)) {
		log_sys_msg("%s() - timerfd_settime('%d'): %s\n", __func__, fd, strerror(errno));
		return 1;
	}
	
	return 0;
}

api_definition(reactor, reactor_epoll) {

	.init = reactor_epoll__init,
	.fini = reactor_epoll__fini,

	.add = reactor_epoll__add,
	.del = reactor_epoll__del,
	.mod = reactor_epoll__mod,

	.new_sig = reactor_epoll__new_sig,
	
	.new_timerfd = reactor_epoll__new_timerfd,
	.arm_timerfd = reactor_epoll__arm_timerfd,
	.disarm_timerfd = reactor_epoll__disarm_timerfd,

	.wait = reactor_epoll__wait,
};
