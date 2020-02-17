
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include <pthread.h>

#include "handler.h"

int event_handler(struct channel *ch)
{
	struct master_info *mi;
	struct  slave_info *si;

	switch (ch->thread_type) {
	
		case THREAD_MASTER: 

			mi = ch->data;
			pthread_cond_signal(&mi->wake_up);
			break;

		default:

			si = ch->data;
			pthread_cond_signal(&si->wake_up);
			break;
	}

	return 0;
}

void slave_visit(void *dptr)
{
	struct slave_info *si = dptr;
	printf("%s() - enter\n", __func__);
	pthread_mutex_lock(&si->mutex);

	printf("Thread '%ld' processed '%d' tasks\n", si->tid, si->counter);
	
	pthread_mutex_unlock(&si->mutex);
}

int signal_handler(struct channel *ch)
{
	struct master_info *mi = ch->data;
	struct list_api *list = mi->list;
	  void *slaves = mi->slaves;

	pthread_mutex_lock(&mi->mutex);

	printf("\n\n\n\n");
	printf("***** INFORMATION *****\n");
	printf("Master thread generated %d tasks\n", mi->counter);

	pthread_mutex_lock(&mi->list_mutex);

	list->walk(&slaves, slave_visit);

	pthread_mutex_unlock(&mi->list_mutex);

	pthread_mutex_unlock(&mi->mutex);
	return 0;
}
