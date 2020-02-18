
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>

#include "compiler.h"

#include "api/list-api.h"
#include "api/mempool-api.h"

#include "utils/logging.h"
#include "utils/load-tool.h"

#define LIST_MP	"mempool0_api"

/* ============================================	*
 *  NODE            NODE            NODE	*
 *  						*
 * |next|--------->|next|--------->|NULL|	*
 * |====|	   |====|          |====|	*
 * |dptr|->|data|  |dptr|->|data|  |dptr|->data	*
 * ============================================	*/

struct list0_node {
	struct list0_node *next;
	  void *dptr;
};

static struct mempool *__list0_mp  = NULL;
static struct mempool_api *mplist0 = NULL;

__attribute__((constructor))
static void list0_init(void)
{
	mplist0 = get_api(LIST_MP);
	if (NULL == mplist0) {
		log_sys_msg("%s() - failed to get '%s'\n", __func__, LIST_MP);
		exit(1);
	}

	__list0_mp = mplist0->new("list0-mempool", sizeof(struct list0_node));

	if (NULL == __list0_mp) {
		log_err_msg("%s() - failed to create new mempool\n", __func__);
		exit(1);
	}
}

static void **list0__add_tail(void **head)
{
	struct list0_node *node =  mplist0->get_block(__list0_mp);
	struct list0_node *tail = *head;
	
	if (unlikely(NULL == node))
		return NULL;

	if (NULL == *head) {
		*head = node;
		node->next = NULL;
		return &node->dptr;
	}

	while (tail->next)
		tail = tail->next;

	tail->next = node;
	node->next = NULL;
	return &node->dptr;
}

static void **list0__add_head(void **head)
{
	struct list0_node *node = mplist0->get_block(__list0_mp);
	
	if (unlikely(NULL == node))
		return NULL;

	if (NULL == *head) {
		*head = node;
		node->next = NULL;
		return &node->dptr;
	}

	node->next = *head;
	*head = node;

	return &node->dptr;
}

static void *list0__get_head(void **head)
{
	struct list0_node *node = *head;
	  void *data;

	if (NULL == node)
		return NULL;

	*head = node->next;
	 data = node->dptr;

	mplist0->put_block(__list0_mp, node);
	return data;
}

static void *list0__get_tail(void **head)
{
	struct list0_node *node = *head;
	struct list0_node *temp = (node) ? node->next : NULL;
	  void *data = NULL;

	if (NULL == node) {
		
		goto __ret;
	};

	if (NULL == temp) {
		
		data = node->dptr;
		mplist0->put_block(__list0_mp, node);
		*head = NULL;
		goto __ret;
	}

	while (1) {
		
		temp = node;
		node = node->next;

		if (NULL == node->next) {

			data = node->dptr;
			mplist0->put_block(__list0_mp, node);
			temp->next = NULL;
			goto __ret;
		}
	}

__ret:
	return data;
}

static void list0__walk(void **head, void (*fvisit)(void *dptr))
{
	struct list0_node *node = *head;

	while (node) {

		fvisit(node->dptr);
		node = node->next;
	}
}

static void list0__del(void **head)
{
	struct list0_node *node = *head;
#if 1
	while (*head) {

		node = *head;
		*head = node->next;
		mplist0->put_block(__list0_mp, node);
	}
#endif

	mplist0->del(__list0_mp);
}

api_definition(list_api, list0_api) {
	.add_tail = list0__add_tail,
	.add_head = list0__add_head,

	.get_head = list0__get_head,
	.get_tail = list0__get_tail,
	.walk     = list0__walk,

	.del      = list0__del,
};
