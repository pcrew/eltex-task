
#ifndef LOGGING_H
#define LOGGING_H

#include <stdio.h>
#include <errno.h>

#include "numeric-types.h"

extern int __log_level;

void set_log_level(int a);

#define __log_msg(log_level, TYPE, ...)				\
do {								\
		if (log_level >= __log_level) {			\
			fprintf(stdout, TYPE __VA_ARGS__);	\
			fflush(stdout);				\
		}						\
} while(0)

#define log_dbg_msg(...) __log_msg(0, "DBG: ", __VA_ARGS__)
#define log_wrn_msg(...) __log_msg(1, "WRN: ", __VA_ARGS__)
#define log_err_msg(...) __log_msg(2, "ERR: ", __VA_ARGS__)
#define log_sys_msg(...) __log_msg(3, "SYS: ", __VA_ARGS__)
#define log_inf_msg(...) __log_msg(4, "INF: ", __VA_ARGS__)
#define log_ops_msg(...) __log_msg(5, "OPS: ", __VA_ARGS__)
#define log_ftl_msg(...) __log_msg(6, "FTL: ", __VA_ARGS__)
#define log_bug_msg(...) __log_msg(7, "BUG: ", __VA_ARGS__)

#endif
