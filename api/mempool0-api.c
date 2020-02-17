
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

#include "compiler.h"
#include "macro-magic.h"
#include "api/mempool-api.h"

#include "utils/logging.h"

#define STRERR	strerror(errno)
#define BLOCKS	1024

struct block {
	struct block *next;
};

struct mempool {

	  char *name;

	struct block *block;
	   u32 block_size;

	   u8 **addr;
	   u32 naddr;
};

static void __mempool0_init_blocks(u8 *addr, u32 blocks_qty, u32 block_size) 
{
	struct block *b;
	    u8 *a = addr;

	while (--blocks_qty) {
		b = (struct block *) a;
		b->next = (struct block *) (a + block_size);
		a += block_size;
	}

	b = (struct block *) a;
	b->next = NULL;

	log_dbg_msg("%s() - blocks successfuly init\n", __func__);
}

static void mempool0__del(struct mempool *mp)
{
	u32 i;

	for (i = 0; i < mp->naddr;i++) {
		if (mp->addr[i])
			free(mp->addr[i]);
	}
	
	free(mp->addr);
	free(mp);
}

static int __mempool0_new_addr(struct mempool *mp)
{
	u8 **addr;
	u8 *block;
       
	addr = realloc(mp->addr, (mp->naddr + 1) * sizeof(u8 *));
	if (addr == NULL) {
		log_sys_msg("%s() - realloc failed: %s\n", __func__, STRERR);
		return 1;
	}

	mp->addr = addr;
	
	block = malloc(BLOCKS * mp->block_size);
	if (NULL == block) {
		log_sys_msg("%s() - malloc() failed: %s\n", __func__, STRERR);
		return 1;
	}

	mp->addr[mp->naddr++] = block;
	mp->block = (struct block *) block;

	__mempool0_init_blocks(block, BLOCKS, mp->block_size);
	return 0;
}

static struct mempool *mempool0__new(char *name, u32 block_size)
{
	struct mempool *mp;

	block_size = max(block_size, sizeof(struct block));
	block_size = ALIGN(block_size, 8);

	mp = malloc(sizeof(struct mempool));
	if (NULL == mp) {
		log_sys_msg("%s(%s) - malloc(): %s\n", __func__, name, STRERR);
	       	return NULL;
	}

	mp->name = (name) ? name: "nameless";
	mp->block_size = block_size;
	mp->addr = NULL;
	mp->naddr = 0;
	
	__mempool0_new_addr(mp);
	return mp;
}

static void *mempool0__get_block(struct mempool *mp, ...)
{
	struct block *b;
	   int error;

	if (NULL == mp->block) {
		
		error = __mempool0_new_addr(mp);
		if (error) {
			log_sys_msg("%s() failed to create addr\n", __func__);
			return NULL;
		}
	}

	b = mp->block;
	mp->block = b->next;
	return b;
}

static void mempool0__put_block(struct mempool *mp, void *ptr, ...)
{
	struct block *d;

	d = mp->block;
	mp->block = (struct block *) ptr;
	mp->block->next = d;
}

static void mempool0__inf(struct mempool *mp)
{
	log_inf_msg("%s() - block size:	%d\n", __func__, mp->block_size);
	log_inf_msg("%s() - naddr:	%d\n", __func__, mp->naddr);
	log_inf_msg("%s() - bytes:	%d\n",
			__func__, mp->naddr * BLOCKS * mp->block_size);
}

api_definition(mempool_api, mempool0_api) {
	.new = mempool0__new,
	.del = mempool0__del,

	.get_block = mempool0__get_block,
	.put_block = mempool0__put_block,
	
	.inf = mempool0__inf,
};
