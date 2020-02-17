
#ifndef DATA_BUFFER_H
#define DATA_BUFFER_H

#define DBUF_IS_INITED	0x0001;

#include "numeric-types.h"

struct dbuf {
	u8 *loc;
	u8 *pos;
	u32 cnt;
	u32 len;
};

#endif
