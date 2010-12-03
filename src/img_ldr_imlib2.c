// -*- indent-tabs-mode: t; tab-width: 8; c-basic-offset: 4; -*-
// vim: sw=4:ts=8:sts=4
#include <stdio.h>
#include <string.h>
#include <Imlib2.h>
#include "mb_graph_engine.h"
#include "mb_tools.h"
#include "mb_paint.h"
#include "mb_img_ldr.h"

/*! \brief Simple image loader.
 *
 */
struct _simple_mb_img_ldr {
    mb_img_ldr_t ldr;
    const char *repo;		/*!< \brief The directory of repository. */
};
typedef struct _simple_mb_img_ldr simple_mb_img_ldr_t;

struct _simple_mb_img_data {
    mb_img_data_t img;
    Imlib_Image img_hdl;
};
typedef struct _simple_mb_img_data simple_mb_img_data_t;

static void simple_mb_img_ldr_img_free(mb_img_data_t *img);

static
mb_img_data_t *simple_mb_img_ldr_load(mb_img_ldr_t *ldr, const char *img_id) {
    simple_mb_img_ldr_t *sldr = (simple_mb_img_ldr_t *)ldr;
    simple_mb_img_data_t *img;
    Imlib_Image img_hdl;
    int w, h;
    void *data;
    char *fname;
    int sz;

    sz = strlen(sldr->repo);
    sz += strlen(img_id);
    fname = (char *)malloc(sz + 2);
	if (img_id[0] != '/')
        strcpy(fname, sldr->repo);
	else
		fname[0] = 0;
    strcat(fname, img_id);
    
    img_hdl = imlib_load_image(fname);
    if(!img_hdl)
	return NULL;
    imlib_context_set_image(img_hdl);
    w = imlib_image_get_width();
    h = imlib_image_get_height();
    data = imlib_image_get_data_for_reading_only();

    img = O_ALLOC(simple_mb_img_data_t);
    if(img == NULL) {
	imlib_free_image();
	return NULL;
    }
    img->img.content = data;
    img->img.w = w;
    img->img.h = h;
    img->img.stride = w * 4;
    img->img.fmt = MB_IFMT_ARGB32;
    img->img.free = simple_mb_img_ldr_img_free;
    img->img_hdl = img_hdl;

    return (mb_img_data_t *)img;
}

static
void simple_mb_img_ldr_img_free(mb_img_data_t *img) {
    simple_mb_img_data_t *simg = (simple_mb_img_data_t *)img;

    imlib_context_set_image(simg->img_hdl);
    imlib_free_image();
    free(img);
}

static
void simple_mb_img_ldr_free(mb_img_ldr_t *ldr) {
    simple_mb_img_ldr_t *defldr = (simple_mb_img_ldr_t *)ldr;

    free((void *)defldr->repo);
}

mb_img_ldr_t *simple_mb_img_ldr_new(const char *img_repository) {
    simple_mb_img_ldr_t *ldr;
    int sz;

    if(img_repository == NULL)
	return NULL;

    ldr = O_ALLOC(simple_mb_img_ldr_t);
    if(ldr == NULL)
	return NULL;

    /*
     * Copy and formalize path of image repository.
     */
    sz = strlen(img_repository);
    ldr->repo = (const char *)malloc(sz + 2);
    if(ldr->repo == NULL) {
	free(ldr);
	return NULL;
    }
    strcpy((char *)ldr->repo, img_repository);
    if(img_repository[sz - 1] != '/' && strlen(img_repository) != 0) {
	((char *)ldr->repo)[sz] = '/';
	((char *)ldr->repo)[sz + 1] = 0;
    }

    ldr->ldr.load = simple_mb_img_ldr_load;
    ldr->ldr.free = simple_mb_img_ldr_free;

    return (mb_img_ldr_t *)ldr;
}
