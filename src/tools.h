#ifndef __TOOLS_H_
#define __TOOLS_H_

typedef struct _elmpool elmpool_t;

extern elmpool_t *elmpool_new(int elm_sz, int inc_num);
extern void *elmpool_elm_alloc(elmpool_t *pool);
extern void elmpool_elm_free(elmpool_t *pool, void *elm);
extern void elmpool_free(elmpool_t *pool);


#define STAILQ(type)				\
    struct {					\
	type *head;				\
	type *tail;				\
    }
#define STAILQ_INIT(q)				\
    do {					\
	(q).head = (q).tail = NULL;		\
    } while(0)
#define STAILQ_CLEAN(q) STAILQ_INIT(q)
#define STAILQ_HEAD(q) ((q).head)
#define STAILQ_TAIL(q) ((q).tail)
#define STAILQ_NEXT(type, field, elm) ((elm)->field)
#define STAILQ_INS(q, type, field, elm)		\
    do {					\
	(elm)->field = (q).head;		\
	(q).head = elm;				\
	if((q).tail == NULL)			\
	    (q).tail = elm;			\
    } while(0)
#define STAILQ_INS_TAIL(q, type, field, elm)	\
    do {					\
	(elm)->field = NULL;			\
	if((q).tail != NULL)			\
	    (q).tail->field = elm;		\
	(q).tail = elm;				\
	if((q).head == NULL)			\
	    (q).head = elm;			\
    } while(0)
#define STAILQ_INS_AFTER(type, field, follow, elm)	\
    do {						\
	(follow)->field = (elm)->field;			\
	(elm)->field = follow;				\
    } while(0)
#define STAILQ_REMOVE(q, type, field, elm)		\
    do {						\
	if((elm) == (q).head) {				\
	    (q).head = (elm)->field;			\
	    if((q).head == NULL)			\
		(q).tail = NULL;			\
	} else {					\
	    type *_stailq_cur = (q).head;		\
	    while(_stailq_cur != NULL &&		\
		  _stailq_cur->field != (elm))		\
		_stailq_cur = _stailq_cur->field;	\
	    if(_stailq_cur != NULL) {			\
		_stailq_cur->field = (elm)->field;	\
		if((q).tail == (elm))			\
		    (q).tail = _stailq_cur;		\
	    }						\
	}						\
    } while(0)


#include <stdlib.h>

#define O_ALLOC(type) ((type *)malloc(sizeof(type)))

#define OFFSET(type, mem) (((void *)&((type *)NULL)->mem) - NULL)
#define MEM2OBJ(var, type, mem) ((type *)((void *)var - OFFSET(type, mem)))
#define OFF2TYPE(obj, off, type) (*(type *)((void *)(obj) + (off)))

#endif /* __TOOLS_H_ */
