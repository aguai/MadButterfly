#include <stdio.h>
#include <string.h>
#include <cairo.h>
#include "mb_types.h"
#include "mb_shapes.h"
#include "mb_img_ldr.h"
#include "mb_tools.h"

#define ASSERT(x)
#define OK 0
#define ERR -1

typedef struct _sh_image {
    shape_t shape;
    
    co_aix x, y;
    co_aix w, h;
    
    mb_img_data_t *img_data;
    cairo_surface_t *surf;
} sh_image_t;

static void sh_image_free(shape_t *shape);

/*! \brief Creae a new image shape.
 *
 * \param img_data is image data whose owner-ship is transfered.
 */
shape_t *rdman_shape_image_new(redraw_man_t *rdman, mb_img_data_t *img_data,
			       co_aix x, co_aix y, co_aix w, co_aix h) {
    sh_image_t *img;
    cairo_format_t fmt;

    img = O_ALLOC(sh_image_t);
    if(img == NULL)
	return NULL;

    memset(img, 0, sizeof(sh_image_t));
    
    img->shape.free = sh_image_free;
    mb_obj_init((mb_obj_t *)img, MBO_IMAGE);
    img->x = x;
    img->y = y;
    img->w = w;
    img->h = h;
    img->img_data = img_data;

    switch(img_data->fmt) {
    case MB_IFMT_ARGB32:
	fmt = CAIRO_FORMAT_ARGB32;
	break;

    case MB_IFMT_RGB24:
	fmt = CAIRO_FORMAT_RGB24;
	break;

    case MB_IFMT_A8:
	fmt = CAIRO_FORMAT_A8;
	break;

    case MB_IFMT_A1:
	fmt = CAIRO_FORMAT_A1;
	break;
    
    case MB_IFMT_RGB16_565:
	fmt = CAIRO_FORMAT_RGB16_565;
	break;
    
    default:
	mb_obj_destroy(img);
	free(img);
	return NULL;
    }
    
    img->surf = cairo_image_surface_create_for_data(img_data->content,
						    fmt,
						    img_data->width,
						    img_data->height,
						    img_data->stride);
    if(img->surf == NULL) {
	mb_obj_destroy(img);
	free(img);
	return NULL;
    }
    
    return (shape_t *)img;
}

void sh_image_free(shape_t *shape) {
    sh_image_t *img = (sh_image_t *)shape;

    mb_obj_destroy(shape);
    MB_IMG_DATA_FREE(img->img_data);
    cairo_surface_destroy(img->surf);
    free(img);
}

void sh_image_transform(shape_t *shape) {
    sh_image_t *img = (sh_image_t *)shape;
    co_aix poses[4][2];
    int i;

    poses[0][0] = img->x;
    poses[0][1] = img->y;
    poses[1][0] = img->x + img->w;
    poses[1][1] = img->y;
    poses[2][0] = img->x + img->w;
    poses[2][1] = img->y + img->h;
    poses[3][0] = img->x;
    poses[3][1] = img->y + img->h;
    for(i = 0; i < 4; i++)
	coord_trans_pos(img->shape.coord, &poses[i][0], &poses[i][1]);
    
    geo_from_positions(sh_get_geo(shape), 4, poses);
}

/*! \brief Draw image for an image shape.
 *
 * \note Image is not rescaled for size of the shape.
 */
void sh_image_draw(shape_t *shape, cairo_t *cr) {
    sh_image_t *img = (sh_image_t *)shape;
    cairo_pattern_t *saved_source;
    cairo_matrix_t matrix, saved_matrix;
    co_aix *aggr;
    
    aggr = coord_get_aggr_matrix(sh_get_coord(shape));
    cairo_matrix_init(&matrix,
		      aggr[0], aggr[3],
		      aggr[1], aggr[4],
		      aggr[2], aggr[5]);

    /* set matrix */
    cairo_get_matrix(cr, &saved_matrix);
    cairo_set_matrix(cr, &matrix);
    
    /* set source */
    saved_source = cairo_get_source(cr);
    cairo_pattern_reference(saved_source);

    /* draw image */
    cairo_set_source_surface(cr, img->surf, 0, 0);
    cairo_paint(cr);
    
    /* restore source */
    cairo_set_source(cr, saved_source);
    cairo_pattern_destroy(saved_source);

    /* restore matrix */
    cairo_set_matrix(cr, &saved_matrix);
}

void sh_image_set(shape_t *shape, co_aix x, co_aix y,
		  co_aix w, co_aix h) {
    sh_image_t *img = (sh_image_t *)shape;

    img->x = x;
    img->y = y;
    img->w = w;
    img->h = h;
}
