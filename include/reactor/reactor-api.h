
#ifndef REACTOR_API_H
#define REACTOR_API_H

#include "numeric-types.h"
#include "api.h"

typedef int (*reactor_event_handler)(void *data);

api_declaration(reactor) {

	void (*init)(u32 nevents);
	void (*fini)(void);

	 int (*add)(int fd, void *dptr);
	 int (*del)(int fd);
	 int (*mod)(int fd, void *dptr, u32 events);

	 int (*new_sig)(int sig);	/* returned new sigfd */

	 int    (*new_timerfd)(void);
	 int    (*arm_timerfd)(int timerfd, int interval_msec);
	 int (*disarm_timerfd)(int timerfd); /* needed? */

	 int (*wait)(void);
};

#endif
