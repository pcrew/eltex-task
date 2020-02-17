
#ifndef MEMPOOL_API_H
#define MEMPOOL_API_H

#include "api.h"
#include "numeric-types.h"

struct mempool;

api_declaration(mempool_api) {
	struct mempool *(*new)(char *name, u32 block_size);
          void 	        (*del)(struct mempool *mp);

          void         *(*get_block)(struct mempool *mp, ...);
	  void          (*put_block)(struct mempool *mp, void *block, ...);	
	  
	  /* information */
	  void		(*inf)(struct mempool *mp);
};

#endif
