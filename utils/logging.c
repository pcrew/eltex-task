
#include <stdio.h>
#include <stdlib.h>

#include "utils/logging.h"

int __log_level = 0;

inline void set_log_level(int a)
{
	__log_level = a;
}
