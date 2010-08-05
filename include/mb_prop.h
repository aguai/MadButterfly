#ifndef __MB_PROP_H_
#define __MB_PROP_H_

#include "mb_tools.h"

/*! \defgroup mb_prop_grp Property
 * \brief A way the modules can set up their own properties on objects.
 *
 * Properties are key-value pairs that are associated with objects.
 * MadButterfly associate a property store for each object (coord or shape)
 * to keep property values.  Every property is identified by a ID; an
 * integer.  Programmer can use a ID to set/get value to/from a property
 * store.  The ID should be unique in a property store.
 *
 * \todo Add a free function pointer on entries to release resources when
 *	the store is destroy. (See: \ref mouse.c)
 *
 * @{
 */

typedef struct _mb_prop_entry mb_prop_entry_t;
typedef struct _mb_prop_store mb_prop_store_t;

struct _mb_prop_entry {
    int id;
    void *value;
    mb_prop_entry_t *next;
};

/*! \brief Property IDs. */
enum {
    PROP_DUMMY,
    PROP_MEVT_OB_CNT,
    PROP_MEVT_OBSERVER,
    PROP_JSOBJ,
    PROP_LAST
};

struct _mb_prop_store {
    elmpool_t *entry_pool;
    STAILQ(mb_prop_entry_t) entries;
};

#define mb_prop_store_init(prop_store, pent_pool)		\
    do {							\
	(prop_store)->entry_pool = pent_pool;			\
	STAILQ_INIT((prop_store)->entries);			\
    } while(0)

void mb_prop_store_destroy(mb_prop_store_t *prop_store);
void *mb_prop_set(mb_prop_store_t *prop_store, int id, void *value);
void *mb_prop_get(mb_prop_store_t *prop_store, int id);
void mb_prop_del(mb_prop_store_t *prop_store, int id);
int mb_prop_has(mb_prop_store_t *prop_store, int id);

/* @} */

#endif /* __MB_PROP_H_ */
