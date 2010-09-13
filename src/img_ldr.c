#include <stdio.h>
#include <string.h>
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
    mbe_surface_t *surf;
};
typedef struct _simple_mb_img_data simple_mb_img_data_t;

static void simple_mb_img_ldr_img_free(mb_img_data_t *img);

static
mb_img_data_t *simple_mb_img_ldr_load(mb_img_ldr_t *ldr, const char *img_id) {
    simple_mb_img_ldr_t *sldr = (simple_mb_img_ldr_t *)ldr;
    simple_mb_img_data_t *img;
    mbe_surface_t *surf;
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
    
    surf = mbe_image_surface_create_from_png(fname);
    if(surf == NULL)
	return NULL;

    img = O_ALLOC(simple_mb_img_data_t);
    if(img == NULL) {
	mbe_surface_destroy(surf);
	return NULL;
    }
    img->img.content = mbe_image_surface_get_data(surf);
    img->img.w = mbe_image_surface_get_width(surf);
    img->img.h = mbe_image_surface_get_height(surf);
    img->img.stride = mbe_image_surface_get_stride(surf);
    img->img.fmt = mbe_image_surface_get_format(surf);
    img->img.free = simple_mb_img_ldr_img_free;
    img->surf = surf;

    return (mb_img_data_t *)img;
}

static
void simple_mb_img_ldr_img_free(mb_img_data_t *img) {
    simple_mb_img_data_t *simg = (simple_mb_img_data_t *)img;
    mbe_surface_destroy((mbe_surface_t *)simg->surf);
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
