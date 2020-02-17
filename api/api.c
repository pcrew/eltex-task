
#include "api.h"
#include "utils/logging.h"
#include "utils/load-tool.h"

void *get_api(char *name) 
{
	return load_tool(NULL, name);
}
