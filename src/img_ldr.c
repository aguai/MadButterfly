#include <stdio.h>
#include <string.h>
#include <cairo.h>
#include "mb_tools.h"
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
    cairo_surface_t *surf;
};
typedef struct _simple_mb_img_data simple_mb_img_data_t;

static void simple_mb_img_ldr_img_free(mb_img_data_t *img);

static
mb_img_data_t *simple_mb_img_ldr_load(mb_img_ldr_t *ldr, const char *img_id) {
    simple_mb_img_ldr_t *sldr = (simple_mb_img_ldr_t *)ldr;
    simple_mb_img_data_t *img;
    cairo_surface_t *surf;
    char *fname;
    cairo_format_t fmt;
    int sz;

    sz = strlen(sldr->repo);
    sz += strlen(img_id);
    fname = (char *)malloc(sz + 2);
    strcpy(fname, sldr->repo);
    strcat(fname, img_id);
    
    surf = cairo_image_surface_create_from_png(fname);
    if(surf == NULL)
	return NULL;

    img = O_ALLOC(simple_mb_img_data_t);
    if(img == NULL) {
	cairo_surface_destroy(surf);
	return NULL;
    }
    img->img.content = cairo_image_surface_get_data(surf);
    img->img.w = cairo_image_surface_get_width(surf);
    img->img.h = cairo_image_surface_get_height(surf);
    img->img.stride = cairo_image_surface_get_stride(surf);
    fmt = cairo_image_surface_get_format(surf);
    switch(fmt) {
    case CAIRO_FORMAT_ARGB32:
	img->img.fmt = MB_IFMT_ARGB32;
	break;
	
    case CAIRO_FORMAT_RGB24:
	img->img.fmt = MB_IFMT_RGB24;
	break;
	
    case CAIRO_FORMAT_A8:
	img->img.fmt = MB_IFMT_A8;
	break;
	
    case CAIRO_FORMAT_A1:
	img->img.fmt = MB_IFMT_A1;
	break;
	
    default:
	cairo_surface_destroy(surf);
	free(img);
	return NULL;
    }
    img->img.free = simple_mb_img_ldr_img_free;
    img->surf = surf;

    return (mb_img_data_t *)img;
}

static
void simple_mb_img_ldr_img_free(mb_img_data_t *img) {
    simple_mb_img_data_t *simg = (simple_mb_img_data_t *)img;
    cairo_surface_destroy((cairo_surface_t *)simg->surf);
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
    if(img_repository[sz - 1] != '/') {
	((char *)ldr->repo)[sz] = '/';
	((char *)ldr->repo)[sz + 1] = 0;
    }
    
    ldr->ldr.load = simple_mb_img_ldr_load;
    ldr->ldr.free = simple_mb_img_ldr_free;
    
    return (mb_img_ldr_t *)ldr;
}
