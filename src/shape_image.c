#include <stdio.h>
#include <string.h>
#include <cairo.h>
#include "mb_types.h"
#include "mb_shapes.h"
#include "mb_img_ldr.h"
#include "mb_tools.h"

/*! \page sh_image_n_image_ldr Image and Image Loader
 *
 * Image (\ref sh_image_t) is a shape to show an image on the output
 * device.  Programmers manipulate object of an image shape to show it
 * at specified position with specified size.  To create a new instance
 * of sh_iamge_t, an image should be specified.  Programmers must have
 * a way to load image from files.  The solution proposed by MadButterfly
 * is image loader (\ref mb_img_ldr_t).
 *
 * Image loader is a repository of image files, programmers give him an
 * ID and get an image returned the loader.  Image loader decodes image
 * files specified IDs and return them in an internal representation.
 * The internal representation of an array of pixels.  Pixels are in
 * order of columns (X-axis) and then row by row (Y-axis).  An pixel
 * can be 32bits, for ARGB, 24bits, for RGB, 8bits, for 8bits Alpha or
 * 256-grey-levels, and 1bits, for bitmap.
 *
 * Every row is padded to round to byte boundary, a rounded row is a stride.
 * Bytes a stride occupied is stride size.  The internal rpresentation
 * is a series of strides.  The data returned by image loader is
 * \ref mb_img_data_t type.  mb_img_data_t::content is data in internal
 * representation.
 *
 * \ref simple_mb_img_ldr_t is a simple implementation of image loader.
 * It is a repository of image files in a directory and sub-directories.
 * ID of an image is mapped to a file in the directory and sub-directories.
 * ID it-self is a relative path relate to root directory of the repository.
 * \ref simple_mb_img_ldr_t handle PNG files only, now.
 *
 * \section get_img_ldr Get an Image Loader for Program
 * redraw_man_t::img_ldr is an image loader assigned by backend.
 * X backend, now, create an instance of simple_mb_img_ldr_t and assigns
 * the instance to redraw_man_t::img_ldr.  Programmers should get
 * image loader assigned for a rdman by calling rdman_img_ldr().
 *
 * \image html image_n_ldr.png
 * \image latex image_n_ldr.eps "Relationship of image and loader" width=10cm
 */

#define ASSERT(x)
#define OK 0
#define ERR -1

/*! \brief Image shape.
 */
typedef struct _sh_image {
    shape_t shape;
    
    co_aix x, y;
    co_aix w, h;
    co_aix poses[4][2];
    
    mb_img_data_t *img_data;
    paint_t *paint;
    redraw_man_t *rdman;
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
    paint_t *paint;

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

    paint = rdman_paint_image_new(rdman, img_data);
    rdman_paint_fill(rdman, paint, (shape_t *)img);
    img->paint = paint;
    img->rdman = rdman;

    return (shape_t *)img;
}

void sh_image_free(shape_t *shape) {
    sh_image_t *img = (sh_image_t *)shape;

    rdman_paint_free(img->rdman, img->paint);
    mb_obj_destroy(shape);
    MB_IMG_DATA_FREE(img->img_data);
    free(img);
}

void sh_image_transform(shape_t *shape) {
    sh_image_t *img = (sh_image_t *)shape;
    mb_img_data_t *img_data;
    co_aix (*poses)[2];
    co_aix img_matrix[6];
    co_aix x_factor, y_factor;
    cairo_matrix_t cmatrix;
    int i;
    
    img_data = img->img_data;
    
    poses = img->poses;
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

    /* Transformation from user space to image space */
    img_matrix[0] = (poses[1][0] - poses[0][0]) / img->w;
    img_matrix[1] = (poses[1][1] - poses[0][1]) / img->w;
    img_matrix[2] = -poses[0][0];
    img_matrix[3] = (poses[3][0] - poses[0][0]) / img->h;
    img_matrix[4] = (poses[3][1] - poses[0][1]) / img->h;
    img_matrix[5] = -poses[0][1];
    if(img->w != img_data->w ||
       img->h != img_data->h) {
	/* Resize image */
	x_factor = img_data->w / img->w;
	img_matrix[0] *= x_factor;
	img_matrix[1] *= x_factor;
	img_matrix[2] *= x_factor;
	y_factor = img_data->h / img->h;
	img_matrix[3] *= y_factor;
	img_matrix[4] *= y_factor;
	img_matrix[5] *= y_factor;
    }
    paint_image_set_matrix(sh_get_fill(shape), img_matrix);
    
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
    
    cairo_move_to(cr, img->poses[0][0], img->poses[0][1]);
    cairo_line_to(cr, img->poses[1][0], img->poses[1][1]);
    cairo_line_to(cr, img->poses[2][0], img->poses[2][1]);
    cairo_line_to(cr, img->poses[3][0], img->poses[3][1]);
    cairo_close_path(cr);
}

/*! \brief Change geometry of an image.
 *
 * Set position and size of an image.
 */
void sh_image_set_geometry(shape_t *shape, co_aix x, co_aix y,
			   co_aix w, co_aix h) {
    sh_image_t *img = (sh_image_t *)shape;

    img->x = x;
    img->y = y;
    img->w = w;
    img->h = h;
}
