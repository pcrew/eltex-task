
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <fcntl.h>
#include <errno.h>

#include <sys/epoll.h>
#include <sys/stat.h>
#include <pthread.h>

#include "reactor/reactor-api.h"

#include "numeric-types.h"
#include "data-buffer.h"
#include "compiler.h"
#include "channel.h"
#include "api.h"

#include "api/list-api.h"

#include "utils/backtrace.h"
#include "utils/logging.h"

#define NTHREADS	4

static struct channel sig_ch = {0};
static struct channel master_thread_ch = {0};

static struct reactor *reactor = NULL;
	  int sigusr1;

static struct master_info mi = {0};

static    int dev_random = -1;

static   char buffer[128];

struct list_api *list = NULL;

  void *free_slaves;
  void *slaves;

int init_sigusr1(void)
{
	   int ret = 0;

	if (NULL == reactor) { /* I'm so pedantic */
		log_ops_msg("reactor api don't loaded\n");
		exit(1);
	}

	printf("%s() - SIGUSR1: %d\n", __func__, SIGUSR1);
	sigusr1 = reactor->new_sig(SIGUSR1);
	if (-1 == sigusr1) {
		log_err_msg("%s() - reactor->new_sig('SIGUSR1') failed: %s\n", __func__, strerror(errno));
		exit(1);
	}

	sig_ch.type = CH_SIGN;
	sig_ch.fd = sigusr1;
	sig_ch.data = &mi;

	ret = reactor->add(sigusr1, &sig_ch);
	if (ret) {
		log_err_msg("%s() - reactor->add(sigusr1) failed: %s\n", __func__, strerror(errno));
		exit(1);
	}

	ret = reactor->mod(sigusr1, &sig_ch, EPOLLIN);
	if (ret) {
		log_err_msg("%s() - reactor->mod(sigusr1) failed: %s\n", __func__, strerror(errno));
		exit(1);
	}

	return ret;
}

void *master_thread_func(void *arg)
{
	struct slave_info *si;

	int ret;
	
	int fd = master_thread_ch.fd;
	u32 rand;

	dev_random = open("/dev/random", O_RDONLY);
	if (-1 == dev_random) {
		log_err_msg("%s() - Can't open 'dev/random'\n", __func__);
		return NULL;
	}

	ret = reactor->add(fd, &master_thread_ch);
	if (ret) {
		log_err_msg("%s() - reactor->add() failed: %s\n", __func__, strerror(errno));
		exit(1);
	}

	mi.counter = 0;

	for (;;) {
		
		pthread_mutex_lock(&mi.mutex);

		ret = read(dev_random, &rand, sizeof(u32));
		if (ret != sizeof(u32)) {
			log_err_msg("%s() - Can't read from 'dev/random'\n", __func__);
			return NULL;
		}

		rand = rand % 1000;

		log_dbg_msg("%s() - rand: %d\n", __func__, rand);
			
		ret = reactor->arm_timerfd(fd, rand + 1);
		if (ret) {
			log_sys_msg("%s() - reactor->arm_timerfd() failed: %s\n", __func__, strerror(errno));
			return NULL;
		}
		
		ret = reactor->mod(fd, &master_thread_ch, EPOLLIN);
		if (ret) {
			log_err_msg("%s() - reactor->mod() failed: %s\n", __func__, strerror(errno));
			exit(1);
		}

		pthread_cond_wait(&mi.wake_up, &mi.mutex);

		sprintf(buffer, "task: %d; time: %d", mi.counter, rand);
		mi.counter++;
	
#if 0	
		pthread_mutex_lock(&mi.list_mutex);

		si = (struct slave_info *) list->get_head(&free_slaves);
		if (NULL == si) {
			/* TODO: Что делать в случае, если все рабы отдыхают - создавать нового или ждать? */
			printf("%s() - slave list empty\n", __func__);
		}

		pthread_mutex_unlock(&mi.list_mutex);
#endif

#if 1
		while (1) { /* Как же глупо, но пока так. :)  Вечером подумаю */

			pthread_mutex_lock(&mi.list_mutex);
			si = (struct slave_info *) list->get_head(&free_slaves);
			
			if (likely(NULL != si)) {
				pthread_mutex_unlock(&mi.list_mutex);
				break;
			}
			
			log_dbg_msg("%s() - wait for free slave\n", __func__);
			pthread_mutex_unlock(&mi.list_mutex);
			sleep(1);
		}
#endif
		pthread_cond_signal(&si->to_work);
		pthread_mutex_unlock(&mi.mutex);
	}



	return NULL;
}

void *slave_thread_func(void *arg)
{
	struct channel *ch = (struct channel *) arg;
	struct slave_info *si = ch->data;
	  void **ref;
	   int ret;
	
	   int fd = ch->fd;

	ret = reactor->add(fd, ch);
	if (ret) {
		log_err_msg("%s() - reactor->add() failed: %s\n", __func__, strerror(errno));
		exit(1);
	}

	si->counter = 0;

	for (;;) {
		
		pthread_mutex_lock(&si->mutex);
		pthread_cond_wait(&si->to_work, &si->mutex);

		printf("%s()/%ld - buffer: %s\n", __func__, pthread_self(), buffer);	
		si->counter++;
		
		ret = reactor->arm_timerfd(fd, 400);
		if (ret) {
			log_sys_msg("%s() - reactor->arm_timerfd() failed: %s\n", __func__, strerror(errno));
			return NULL;
		}
		
		ret = reactor->mod(fd, ch, EPOLLIN);
		if (ret) {
			log_err_msg("%s() - reactor->mod() failed: %s\n", __func__, strerror(errno));
			exit(1);
		}

		pthread_cond_wait(&si->wake_up, &si->mutex);

		pthread_mutex_lock(&mi.list_mutex);

		ref = list->add_tail(&free_slaves);
		*ref = si;	

#if 0	/* Сложность добавления в голову - 0(1), в хвост - O(n):
	   	в первом случае - следуя Колмогорову, - почти наверное буду работать только первые два потока, остальные будут курить;
		во втором случае потоки будут вызываться последовательно */

		ref = list->add_head(&free_slaves);
		*ref = si;
#endif

		pthread_mutex_unlock(&mi.list_mutex);
		pthread_mutex_unlock(&si->mutex);		
	}

	return NULL;
}

int init_master_thread(void)
{
	pthread_t tid;
	      int ret;
	      int fd;
	
	if (NULL == reactor) {
		log_ops_msg("%s() - reactor api don't loaded\n", __func__);
		exit(1);
	}

	ret = pthread_create(&tid, NULL, master_thread_func, NULL);
	if (ret) {
		log_sys_msg("%s() - pthread_create(): %s\n", __func__, strerror(errno));
		return 1;
	}

	fd = reactor->new_timerfd();
	if (-1 == fd) {
		log_sys_msg("%s() - reactor->new_timerfd() failed: %s\n", __func__, strerror(errno));
		return 1;
	}

	ret = pthread_mutex_init(&mi.mutex, NULL);
	if (ret) {
		log_sys_msg("%s() - pthread_mutex_init() failed\n", __func__);
		return 1;
	}

	ret = pthread_cond_init(&mi.wake_up, NULL);
	if (ret) {
		log_sys_msg("%s() - pthread_cond_init() failed\n", __func__);
		return 1;
	}

	master_thread_ch.fd = fd;
	master_thread_ch.type = CH_TICK;
	master_thread_ch.thread_type = THREAD_MASTER;
	master_thread_ch.data = &mi;
	
	mi.tid = tid;
	mi.list = list;
	mi.free_slaves = free_slaves;
	mi.slaves = slaves;
	
	return 0;
}

int init_slave_threads(void)
{
	struct slave_info *si;
	struct channel *ch;

	void **ref;

	int ret;
	int fd;
	int i;

	for (i = 0; i < NTHREADS; i++) {

		ch = malloc(sizeof(struct channel));
		si = malloc(sizeof(struct slave_info));
		if (NULL == si || NULL == ch) {
			log_sys_msg("%s() - malloc(#%d) failed\n", __func__, i);
			return 1;
		}

		ret = pthread_create(&si->tid, NULL, slave_thread_func, ch);
		if (ret) {
			log_sys_msg("%s() - pthread_create(): %s\n", __func__, strerror(errno));
			return 1;
		}

		fd = reactor->new_timerfd();
		if (-1 == fd) {
			log_sys_msg("%s() - reactor->new_timerfd() failed: %s\n", __func__, strerror(errno));
			return 1;
		}

		ret = pthread_mutex_init(&si->mutex, NULL);
		if (ret) {
			log_sys_msg("%s() - pthread_mutex_init() failed\n", __func__);
			return 1;
		}


		ret = pthread_cond_init(&si->wake_up, NULL);
		if (ret) {
			log_sys_msg("%s() - pthread_cond_init() failed\n", __func__);
			return 1;
		}

		ch->fd = fd;
		ch->type = CH_TICK;
		ch->thread_type = THREAD_SLAVE;
		ch->data = si;

		si->mi = &mi;
		ref = list->add_head(&free_slaves);
		*ref = si;

		ref = list->add_head(&slaves);
		*ref = si;
	}
	
	return 0;	      
}

int main()
{
	int ret = 0;
	
	ret = bt_init(-1);
	if (ret) {
		log_err_msg("bt_init() failed: %s\n", strerror(errno));
		exit(1);
	}

	log_inf_msg("Program has been started. PID: '%d'\n", getpid());

	list = get_api("list0_api");
	if (NULL == list) {
		log_ops_msg("Failed to get 'list0_api'\n");
		exit(1);
	}

	reactor = get_api("reactor_epoll");
	if (NULL == reactor) {
		log_ops_msg("Failed to get 'reactor_epoll'\n");
		exit(1);
	}

	reactor->init(32);

	ret = init_sigusr1();
	if (ret) {
		log_err_msg("init_sigusr1() - failed\n");
		exit(1);
	}

	ret = init_slave_threads();
	if (ret) {
		log_err_msg("init_slave_threads() - failed\n");
		exit(1);
	}

	ret = init_master_thread();
	if (ret) {
		log_err_msg("init_threads() - failed\n");
		exit(1);
	}

	for (;;) {
		reactor->wait();
	}

	return 0;
}
