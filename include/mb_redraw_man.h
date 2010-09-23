// -*- indent-tabs-mode: t; tab-width: 8; c-basic-offset: 4; -*-
// vim: sw=4:ts=8:sts=4
#ifndef __REDRAW_MAN_H_
#define __REDRAW_MAN_H_

#include "mb_graph_engine.h"
#include "mb_tools.h"
#include "mb_types.h"
#include "mb_observer.h"
#include "mb_img_ldr.h"
#include "mb_timer.h"

typedef struct _redraw_man redraw_man_t;

typedef void (*free_func_t)(redraw_man_t *rdman, void *obj);
struct _free_obj {
    void *obj;
    free_func_t free_func;
};
typedef struct _free_obj free_obj_t;
struct _free_objs {
    int num, max;
    free_obj_t *objs;
};
typedef struct _free_objs free_objs_t;

DARRAY(coords, coord_t *);
DARRAY(geos, geo_t *);

/*! \brief Manage redrawing of shapes (graphic elements).
 *
 * Every coord_t and geo_t object is assigned with a unique
 * incremental order.  The order is a unsigned integer.
 * Every time a new coord_t or geo_t object is added, it is
 * assigned with a order number that 1 bigger than last one
 * until reaching maximum of unsigned integer.
 * When a maximum is meet, all coord_t or geo_t objects
 * are reasigned with a new order number from 1.  It means
 * order numbers that have been assigned and then removed
 * later are recycled.
 *
 * Dirty flag is clear when the transformation matrix of a coord
 * object been recomputed or when a geo_t objects been redrawed.
 */
struct _redraw_man {
    unsigned int next_coord_order;
    int n_coords;
    coord_t *root_coord;

    elmpool_t *geo_pool;
    elmpool_t *coord_pool;
    elmpool_t *shnode_pool;
    elmpool_t *sh_path_pool;
    elmpool_t *sh_rect_pool;
    elmpool_t *observer_pool;
    elmpool_t *subject_pool;
    elmpool_t *paint_color_pool;
    elmpool_t *pent_pool;
    elmpool_t *coord_canvas_pool;

    coords_t dirty_coords;
    geos_t dirty_geos;
    int n_dirty_areas;		/*!< \brief Number of all dirty areas. */

    geos_t gen_geos;
    coords_t zeroing_coords;

    STAILQ(shape_t) shapes;	/*!< \brief All managed shapes.  */
    STAILQ(paint_t) paints;	/*!< \brief All managed paints. */

    free_objs_t free_objs;

    mbe_t *cr;
    mbe_t *backend;

    ob_factory_t ob_factory;

    subject_t *redraw;		/*!< \brief Notified after redrawing. */
    subject_t *addrm_monitor;	/*!< \brief Monitor adding/removing observers
				 *	    to/from mouse event subjects.
				 *	    \see addrm_monitor_hdlr()
				 */
    mb_obj_t *last_mouse_over;
    void *rt;                  /*!< \brief This is a pointer to the current
                                *          graphic backend.
				*          \see rdman_attach_backend()
				*/
    mb_prop_store_t props;
    mb_img_ldr_t *img_ldr;	/*!< \brief Image Loader.
				 *	This is initialized by backend.
				 */
    co_aix w, h;		/*!< \brief Size of viewport
				 *	This is initialized by backend.
				 */
};

extern int redraw_man_init(redraw_man_t *rdman, mbe_t *cr,
			   mbe_t *backend);
extern void redraw_man_destroy(redraw_man_t *rdman);
extern int rdman_find_overlaid_shapes(redraw_man_t *rdman,
				      geo_t *geo,
				      geo_t ***overlays);
extern int rdman_add_shape(redraw_man_t *rdman,
			   shape_t *shape, coord_t *coord);
/*! \brief Make a shape been managed by a redraw manager. */
#define rdman_shape_man(rdman, shape)					\
    do {								\
	mb_prop_store_init(&((mb_obj_t *)(shape))->props,		\
			   (rdman)->pent_pool);				\
	STAILQ_INS_TAIL(rdman->shapes, shape_t, sh_next, shape);	\
	if(rdman->last_mouse_over == (mb_obj_t *)(shape))		\
	    rdman->last_mouse_over = NULL;				\
	mb_prop_store_init(&((mb_obj_t *)(shape))->props,		\
			   (rdman)->pent_pool);				\
    } while(0)
extern int rdman_shape_free(redraw_man_t *rdman, shape_t *shape);

extern int rdman_paint_free(redraw_man_t *rdman, paint_t *paint);

extern coord_t *rdman_coord_new(redraw_man_t *rdman, coord_t *parent);
extern int rdman_coord_free(redraw_man_t *rdman, coord_t *coord);
extern int rdman_coord_subtree_free(redraw_man_t *rdman, coord_t *subtree);
extern int rdman_coord_changed(redraw_man_t *rdman, coord_t *coord);
extern int rdman_shape_changed(redraw_man_t *rdman, shape_t *shape);
extern int rdman_redraw_changed(redraw_man_t *rdman);
extern int rdman_redraw_all(redraw_man_t *rdman);
extern int rdman_redraw_area(redraw_man_t *rdman, co_aix x, co_aix y,
			     co_aix w, co_aix h);
extern geo_t *rdman_geos(redraw_man_t *rdman, geo_t *last);
#define rdman_shapes(rdman, last_shape)					\
    geo_get_shape_safe(rdman_geos(rdman, sh_get_geo_safe(last_shape)))
extern int rdman_force_clean(redraw_man_t *rdman);
extern shnode_t *shnode_new(redraw_man_t *rdman, shape_t *shape);
#define shnode_free(rdman, node) elmpool_elm_free((rdman)->shnode_pool, node)
#define shnode_list_free(rdman, q)				\
    do {							\
	shnode_t *__node, *__last;				\
	__last = STAILQ_HEAD(q);				\
	if(__last == NULL) break;				\
	for(__node = STAILQ_NEXT(shnode_t, next, __last);	\
	    __node != NULL;					\
	    __node = STAILQ_NEXT(shnode_t, next, __node)) {	\
	    shnode_free(rdman, __last);				\
	    __last = __node;					\
	}							\
	shnode_free(rdman, __last);				\
    } while(0)
#define _rdman_paint_child(rdman, paint, shape)		\
    do {						\
	shnode_t *__node;				\
	if((shape)->fill != (paint) &&			\
	   (shape)->stroke != (paint)) {		\
	    __node = shnode_new(rdman, shape);		\
	    STAILQ_INS_TAIL((paint)->members,		\
			    shnode_t, next, __node);	\
	}						\
    } while(0)
extern void _rdman_paint_real_remove_child(redraw_man_t *rdman,
					   paint_t *paint,
					   shape_t *shape);
#define _rdman_paint_remove_child(rdman, paint, shape)		\
    do {							\
	if((shape)->fill == (shape)->stroke &&			\
	   (shape)->stroke == (paint))				\
	    break;						\
	_rdman_paint_real_remove_child(rdman, paint, shape);	\
    } while(0)
#define rdman_paint_fill(rdman, paint, shape)			\
    do {							\
	if((shape)->fill == paint)				\
	    break;						\
	if((shape)->fill)					\
	    _rdman_paint_remove_child(rdman, (shape)->fill,	\
				      shape);			\
	if(paint)						\
	    _rdman_paint_child(rdman, paint, shape);		\
	(shape)->fill = paint;					\
    } while(0)
#define rdman_paint_stroke(rdman, paint, shape)			\
    do {							\
	if((shape)->stroke == paint)				\
	    break;						\
	if((shape)->stroke)					\
	    _rdman_paint_remove_child(rdman, (shape)->stroke,	\
				      shape);			\
	if(paint)						\
	    _rdman_paint_child(rdman, paint, shape);		\
	(shape)->stroke = paint;				\
    } while(0)
extern int rdman_paint_changed(redraw_man_t *rdman, paint_t *paint);

extern shape_t *find_shape_at_pos(redraw_man_t *rdman,
				  co_aix x, co_aix y, int *in_stroke);
extern int mb_obj_pos_is_in(redraw_man_t *rdman, mb_obj_t *obj,
			    co_aix x, co_aix y, int *in_stroke);
extern int mb_objs_are_overlay(redraw_man_t *rdman,
			       mb_obj_t *obj1, mb_obj_t *obj2);
#define rdman_get_ob_factory(rdman) (&(rdman)->ob_factory)
#define rdman_get_redraw_subject(rdman) ((rdman)->redraw)
#define rdman_get_root(rdman) ((rdman)->root_coord)
#define rdman_get_cr(rdman) ((rdman)->cr)
#define rdman_get_gen_geos(rdman) (&(rdman)->gen_geos)
extern int rdman_add_gen_geos(redraw_man_t *rdman, geo_t *geo);
#define rdman_get_shape_gl(rdman, idx)			\
    geo_get_shape(rdman_get_gen_geos(rdman)->ds[idx])
#define rdman_add_shape_gl(rdman, shape)			\
    rdman_add_gen_geos(rdman, sh_get_geo(shape))
#define rdman_shape_gl_len(rdman)		\
    rdman_get_gen_geos(rdman)->num
#define rdman_clear_shape_gl(rdman)		\
    DARRAY_CLEAN(rdman_get_gen_geos(rdman))
#define rdman_prop_store(rdman) ((rdman)->props)
#define rdman_img_ldr(rdman) ((rdman)->img_ldr)
#define rdman_set_img_ldr(rdman, ldr)		\
    do { (rdman)->img_ldr = ldr; } while(0)

/*! \brief Attach backend to the redraw manager so that we can hide the backend from the users.
 *
 */
#define rdman_attach_backend(rdman,backend) (((rdman)->rt)=(backend))
/*! \brief Load sprite dymanicly from the shared object module.
 *
 * The search path can be changed by sprite_set_search_path. The name
 * can have a relative path in the front of it.
 * This function will search the object in the current working directory
 * and then search the system search path.
 */
extern mb_sprite_t *sprite_load(const char *name, redraw_man_t *rdman,
				coord_t *root);

/*! \brief Set the search path of dymanic object loading.
 *
 */
extern void sprite_set_search_path(char *path);

extern paint_t *rdman_img_ldr_load_paint(redraw_man_t *rdman,
					 const char *img_id);

typedef void (*mb_eventcb_t )(int fd,void *arg);
#define MONITOR_READ   1
#define MONITOR_WRITE  2

/*! \brief The backend engine mb_backend_t is used to define the interface to realize the MB.
 *
 * A backend is used to receive events from the system. The MB does not define the backend by itself.
 * Instead, it define an interface which allow the lower layer to implement the event system. Each
 * backend need to provides the following events.
 *
 * - keyboard event
 * - timer event
 * - image loader(?)
 * - render manager(?)
 */
typedef struct {

    void *(*init)(const char *display,int w,int h);
    void (*free)(void *be);
    void (*add_event)(void *be,int type, int fd, mb_eventcb_t f,void *arg);
    void (*remove_event)(void *be,int type, int fd);
    void (*loop)(void *be);
    subject_t *(*kbevents)(void *be);
    redraw_man_t *(*rdman)(void *be);
    mb_tman_t *(*tman)(void *be);
    ob_factory_t *(*factory)(void *be);
    mb_img_ldr_t *(*loader)(void *be);
} mb_backend_t;

extern mb_backend_t backend;

#endif /* __REDRAW_MAN_H_ */
