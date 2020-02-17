
#ifndef LIST_API
#define LIST_API

#include "api.h"
#include "numeric-types.h"

#define LIST_ADD_TAIL(data, list, list_api)		\
do {							\
	void **__ref = list_api->add_tail(&(list));	\
	      *__ref = (__ref) ? (void *) (data) : NULL;	\
} while (0)

#define LIST_ADD_HEAD(data, list, list_api)		\
do {							\
	void **__ref = list_api->add_head(&list);	\
	      *__ref = (__ref) ? (void *) data : NULL;	\
} while (0)

api_declaration(list_api) {

	void **(*add_tail)(void **head);
	void **(*add_head)(void **head);

	void  *(*get_head)(void **head);
	void  *(*get_tail)(void **head);

	void   (*walk)(void **head, void (*fvisit)(void *dptr));
	
	void   (*del)(void **head);
};

#endif
