
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <dlfcn.h>

#include "utils/logging.h"
#include "utils/load-tool.h"

inline void *load_tool(void *handle, char *symname)
{
	void *tool;
	char *err;

	tool = dlsym(handle, symname);
	err = dlerror();
	if (err) {
		log_sys_msg("%s() - dlsym('%s'): %s\n", __func__, symname, err);
		return NULL;
	}

	return tool;
}
