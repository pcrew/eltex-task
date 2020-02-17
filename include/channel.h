
#ifndef CHANNEL_H
#define CHANNEL_H

#include <sys/signalfd.h>

#include "numeric-types.h"
#include "thread-info.h"

#define CH_NONE		0x0000
#define CH_TICK		0x0001
#define CH_SIGN		0x0002

#define THREAD_MASTER	0x0001
#define THREAD_SLAVE	0x0002

struct channel {

	u8 type; /* TE_XXXX */
	u8 thread_type; /*   */
	u32 fd;
	u32 flags;
	u32 revents;
	
	void *data;
};

#endif
