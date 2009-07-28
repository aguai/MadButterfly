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
#define STAILQ_FOR_EACH(q, type, field, elm)	\
    for((elm) = (q).head;			\
	(elm) != NULL;				\
	(elm) = (elm)->field)

/*! \defgroup darray Dynamic Array
 *
 * DARRAY is a dynamic sized array/list, it's length is a variable.
 * It is extended, automatically, if it is full and more elemnts are
 * putted in.
 *
 * Users of DARRAY must declare a new type to store data.  The way to
 * declear a new type is to invoke DARRAY() with paramters of name of
 * type and type of data to be stored in.  The new storage type is named
 * with foo_t where foo is the name you pass in.
 *
 * DARRAY_DEFINE() is inovked to define foo_add() function; foo is name
 * of storage type.  You can call foo_add() to add a data element
 * into a storage object.
 *
 * Get ith element in a storage object, use
 * \code
 * obj->ds[i]
 * \endcode
 *
 * To loop over elements in a storage object, us
 * \code
 * for(i = 0; i < obj->num; i++) {
 *	v = obj->ds[i];
 *	......
 * }
 * \endcode
 * @{
 */
/*! \brief Declare a DARRAY storage type.
 *
 * \param name is name of storage type.
 * \param type is type of data elements that will be stored in.
 *
 * Type of <name>_t is defined by the macro.  It is used to define a
 * storage object to contain data elements.
 */
#define DARRAY(name, type)				\
    struct _ ## name {					\
	int max, num;					\
	type *ds;					\
    };							\
    typedef struct _ ## name name ## _t
#define DARRAY_DEFINE(name, type)			\
    static int name ## _add(name ## _t *da, type v) {	\
	type *new_ds;					\
	int max;					\
	if(da->num >= (da)->max) {			\
	    max = (da)->max + 32;			\
	    new_ds = realloc(da->ds,			\
			     max * sizeof(type));	\
	    if(new_ds == NULL) return -1;		\
	    da->ds = new_ds;				\
	    da->max = max;				\
	}						\
	da->ds[da->num++] = v;				\
	return 0;					\
    }
#define DARRAY_DEFINE_ADV(name, type)			\
    static int name ## _adv(name ## _t *da, int n) {	\
	type *new_ds;					\
	int max;					\
	if((da->num + n) > (da)->max) {			\
	    max = ((da)->num + n + 31) & ~0x1f;		\
	    new_ds = realloc(da->ds,			\
			     max * sizeof(type));	\
	    if(new_ds == NULL) return -1;		\
	    da->ds = new_ds;				\
	    da->max = max;				\
	}						\
	da->num += n;					\
	return 0;					\
    }
#define DARRAY_CLEAN(da) do { (da)->num = 0; } while(0)
#define DARRAY_INIT(da) \
    do { (da)->num = (da)->max = 0; (da)->ds = NULL; } while(0)
#define DARRAY_DESTROY(da) do { if((da)->ds) free((da)->ds); } while(0)
/* @} */

#include <stdlib.h>

#define O_ALLOC(type) ((type *)malloc(sizeof(type)))

#define OFFSET(type, mem) (((void *)&((type *)NULL)->mem) - NULL)
#define MEM2OBJ(var, type, mem) ((type *)((void *)var - OFFSET(type, mem)))
#define OFF2TYPE(obj, off, type) (*(type *)((void *)(obj) + (off)))

#define MB_MAX(a, b) ((a) > (b)? (a): (b))
#define MB_MIN(a, b) ((a) < (b)? (a): (b))

#endif /* __TOOLS_H_ */
