
#ifndef THREAD_INFO_H
#define THREAD_INFO_H

#include <pthread.h>

#include "api/list-api.h"
#include "numeric-types.h"

#define MASTER_THREAD	0
#define  SLAVE_THREAD	1

#define THREAD_WAIT	0x0000
#define THREAD_WORK	0x0001

struct master_info {

	pthread_t tid;
	pthread_cond_t wake_up;
	
	u32 counter;

	pthread_mutex_t mutex;
	pthread_mutex_t list_mutex;

	char *buf;

	struct list_api *list;
	  void *free_slaves;
	  void *slaves;	/* all slaves */
};

struct slave_info {

	pthread_t tid;

	pthread_mutex_t mutex;
	pthread_cond_t wake_up;
	pthread_cond_t to_work;
	u32 counter;

	struct master_info *mi;
};

#endif
