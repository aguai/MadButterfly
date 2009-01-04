#include "mb_prop.h"

#define ASSERT(x)


static
mb_prop_entry_t *_mb_prop_find(mb_prop_store_t *prop_store, int id) {
    mb_prop_entry_t *entry;

    for(entry = STAILQ_HEAD(prop_store->entries);
	entry != NULL;
	entry = STAILQ_NEXT(mb_prop_entry_t, next, entry)) {
	if(entry->id == id)
	    return entry;
    }
    
    return NULL;
}

void mb_prop_store_destroy(mb_prop_store_t *prop_store) {
    mb_prop_entry_t *entry, *last;

    last = STAILQ_HEAD(prop_store->entries);
    if(last == NULL)
	return;

    for(entry = STAILQ_NEXT(mb_prop_entry_t, next, entry);
	entry != NULL;
	entry = STAILQ_NEXT(mb_prop_entry_t, next, entry)) {
	STAILQ_REMOVE(prop_store->entries, mb_prop_entry_t, next, last);
	elmpool_elm_free(prop_store->entry_pool, last);
	last = entry;
    }
    STAILQ_REMOVE(prop_store->entries, mb_prop_entry_t, next, last);
    elmpool_elm_free(prop_store->entry_pool, last);
}

void *mb_prop_set(mb_prop_store_t *prop_store, int id, void *value) {
    mb_prop_entry_t *entry;
    void *old;

    entry = _mb_prop_find(prop_store, id);
    if(entry) {
	old = entry->value;
	entry->value = value;
	return old;
    }

    entry = elmpool_elm_alloc(prop_store->entry_pool);
    ASSERT(entry != NULL);
    entry->id = id;
    entry->value = value;
    STAILQ_INS(prop_store->entries, mb_prop_entry_t, next, entry);

    return NULL;
}

void *mb_prop_get(mb_prop_store_t *prop_store, int id) {
    mb_prop_entry_t *entry;

    entry = _mb_prop_find(prop_store, id);
    if(entry)
	return entry->value;

    return NULL;
}

void mb_prop_del(mb_prop_store_t *prop_store, int id) {
    mb_prop_entry_t *entry;

    entry = _mb_prop_find(prop_store, id);
    if(entry)
	STAILQ_REMOVE(prop_store->entries, mb_prop_entry_t, next, entry);
}

int mb_prop_has(mb_prop_store_t *prop_store, int id) {
    mb_prop_entry_t *entry;

    entry = _mb_prop_find(prop_store, id);
    if(entry)
	return 1;
    
    return 0;
}
