#include <stdio.h>
#include "mb_img_ldr.h"

static mb_img_data_t *img_ldr_dummy_load(mb_img_ldr_t *ldr,
					 const char *img_id);
static void img_ldr_dummy_free(mb_img_ldr_t *ldr);

static mb_img_ldr_t img_ldr = {
    img_ldr_dummy_load,
    img_ldr_dummy_free
};

#ifndef ERR
#include <stdio.h>
#include <stdlib.h>
#define ERR(msg) do { fprintf(stderr, __FILE__ ":%d: %s", __LINE__, msg); abort(); } while(0)
#endif
#ifndef NOT_IMPLEMENT
#define NOT_IMPLEMENT(func)			\
    ERR(func " is not impmemented\n")
#endif

static mb_img_data_t *
img_ldr_dummy_load(mb_img_ldr_t *ldr, const char *img_id) {
    NOT_IMPLEMENT("img_ldr_dummy_load");
    return NULL;
}

static void
img_ldr_dummy_free(mb_img_ldr_t *ldr) {
    NOT_IMPLEMENT("img_ldr_dummy_free");
}

mb_img_ldr_t *
simple_mb_img_ldr_new(const char *img_repository) {
    return &img_ldr;
}

