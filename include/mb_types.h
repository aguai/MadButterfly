#ifndef __MB_TYPES_H_
#define __MB_TYPES_H_

#include "mb_graph_engine.h"
#include "mb_tools.h"
#include "mb_observer.h"
#include "mb_prop.h"

typedef struct _shape shape_t;
typedef struct _geo geo_t;
typedef struct _area area_t;
typedef struct _shnode shnode_t;
typedef struct _paint paint_t;
typedef struct _mb_obj mb_obj_t;
typedef struct _mb_sprite mb_sprite_t;
/*! \todo Replace mbe_t with canvas_t. */
typedef mbe_t canvas_t;

struct _redraw_man;

/* \defgroup mb_obj_grp Object type
 * @{
 */
/*! \brief MadButterfly object.
 *
 * All objects (coord and shapes) should have mb_obj_t as first member
 * variable.  obj_type is used to identify type of an object.  Please,
 * use MBO_TYPE() to return this value.  MBO_TYPE() will type-casting the
 * object to mb_obj_t and return obj_type.
 *
 * mb_obj_t should be initialized with mb_obj_init() and destroied with
 * mb_obj_destroy().
 *
 * We have defined a set of convienent API which will wrap the coord_t or shape_t API accoridng to its type.
 * Please refer to http://www.assembla.com/wiki/show/dFrSMOtDer3BZUab7jnrAJ/MBAF_Object for the details. This
 * API is designed for regular programmers which can be used to change some common properties of objects without
 * checking its type.
 */
struct _mb_obj {
    int obj_type;		/*!< \brief Type of a MadButterfly object. */
    mb_prop_store_t props;	/*!< Initialized by rdman. */
};

enum { MBO_DUMMY,
       MBO_COORD,
       MBO_SHAPES=0x1000,	/*! \note Don't touch this.  */
       MBO_PATH,
       MBO_TEXT,
       MBO_RECT,
       MBO_IMAGE,
       MBO_STEXT
};
#define MBO_CLASS_MASK 0xf000
#define MBO_CLASS(x) (((mb_obj_t *)(x))->obj_type & MBO_CLASS_MASK)
/*! \brief Return type of a MadBufferly object. */
#define MBO_TYPE(x) (((mb_obj_t *)(x))->obj_type)
#define IS_MBO_SHAPES(obj) (MBO_CLASS(obj) == MBO_SHAPES)
#define IS_MBO_COORD(obj) (MBO_TYPE(obj) == MBO_COORD)
#define mb_obj_init(obj, type)					\
    do {							\
	((mb_obj_t *)(obj))->obj_type = type;			\
    } while(0)
#define mb_obj_destroy(obj)
#define mb_obj_prop_store(obj) (&(obj)->props)


/* @} */

/*! \brief Base of paint types.
 *
 * Paints should be freed by users by calling rdman_paint_free() of
 * the paint.
 *
 * To define a foo paint, it should define a rdman_paint_foo_new()
 * function.  It return a paint object.
 *
 * \todo move member functions to a seperate structure and setup a
 * singleton for each paint type.
 */
struct _paint {
    int pnt_type;
    int flags;
    void (*prepare)(paint_t *paint, mbe_t *cr);
    void (*free)(struct _redraw_man *rdman, paint_t *paint);
    STAILQ(shnode_t) members;
    paint_t *pnt_next;		/*!< \brief Collect all paints of a rdman. */
};
enum { MBP_DUMMY,
       MBP_COLOR,
       MBP_LINEAR,
       MBP_RADIAL,
       MBP_IMAGE
};

#define PNTF_FREE 0x1

struct _shnode {
    shape_t *shape;
    shnode_t *next;
};

struct _area {
    co_aix x, y;
    co_aix w, h;
};

/*! \brief Geometry data of a shape or a group of shape.
 */
struct _geo {
#ifdef GEO_ORDER
    unsigned int order;
#endif
    unsigned int flags;
    shape_t *shape;
    geo_t *coord_next;		/*!< \brief Link all member geos together. */

    area_t *cur_area, *last_area;
    area_t areas[2];

    subject_t *mouse_event;
};
#define GEF_DIRTY 0x1
#define GEF_HIDDEN 0x2		/*!< The geo is hidden. */
#define GEF_FREE 0x4
#define GEF_OV_DRAW 0x8		/*!< To flag drawed for a overlay testing. */

extern int areas_are_overlay(area_t *r1, area_t *r2);
extern void area_init(area_t *area, int n_pos, co_aix pos[][2]);
#define _in_range(a, s, w) ((a) >= (s) && (a) < ((s) + (w)))
#define area_pos_is_in(area, _x, _y)		\
    (_in_range(_x, (area)->x, (area)->w) &&	\
     _in_range(_y, (area)->y, (area)->h))
extern void geo_init(geo_t *g);
extern void geo_from_positions(geo_t *g, int n_pos, co_aix pos[][2]);
extern void geo_mark_overlay(geo_t *g, int n_others, geo_t **others,
			     int *n_overlays, geo_t **overlays);
#define geo_get_shape(g) ((g)->shape)
#define geo_get_shape_safe(g) ((g)? (g)->shape: NULL)
#define geo_set_shape(g, sh) do {(g)->shape = sh;} while(0)
#define geo_pos_is_in(g, _x, _y) area_pos_is_in((g)->cur_area, _x, _y)
#define geo_get_area(g) ((g)->cur_area)
#define geo_get_flags(g, mask) ((g)->flags & (mask))
#define geo_set_flags(g, mask) do {(g)->flags |= mask;} while(0)
#define geo_clear_flags(g, mask) do {(g)->flags &= ~(mask);} while(0)
#define geo_get_coord(g) sh_get_coord(geo_get_shape(g))

/*! \defgroup coord Coordination
 * @{
 */
typedef struct _coord coord_t;

DARRAY(areas, area_t *);

/*! \brief Canvas information for a coord.
 */
typedef struct _coord_canvas_info {
    coord_t *owner;		/*!< Cached one or opacity == 1 */
    canvas_t *canvas;
    areas_t dirty_areas;	/*!< \brief Areas should be updated
				 * in canvas.
				 */
    area_t aggr_dirty_areas[2];	/*!< Used to aggregate updates to parent. */
    area_t cached_dirty_area;	/*!< Used to dirty an area in cached space. */
    area_t owner_mems_area;	/*!< \brief The area is covered by members
				 * of owner.
				 */
} coord_canvas_info_t;

/*! \brief A coordination system.
 *
 * It have a transform function defined by matrix to transform
 * coordination from source space to target space.
 * Source space is where the contained is drawed, and target space
 * is where the coordination of parent container of the element
 * represented by this coord object.
 *
 * \dot
 * digraph G {
 * graph [rankdir=LR];
 * root -> child00 -> child10 -> child20 [label="children" color="blue"];
 * child00 -> child01 -> child02 [label="sibling"];
 * child10 -> child11 [label="sibling"];
 * }
 * \enddot
 */
struct _coord {
    mb_obj_t obj;
    unsigned int order;
    unsigned int flags;		/*!< \sa \ref coord_flags */
    co_aix opacity;
    /*! Own one or inherit from an ancestor.
     * Setup it when clean coords.
     * \sa
     * - \ref COF_OWN_CANVAS
     * - \ref redraw
     */
    coord_canvas_info_t *canvas_info;
    area_t *cur_area, *last_area;
    area_t areas[2];

    co_aix matrix[6];
    co_aix aggr_matrix[6];

    struct _coord *parent;
    STAILQ(struct _coord) children;
    struct _coord *sibling;
    unsigned int before_pmem;	/*!< \brief The coord is before nth member
				 * of parent. */

    int num_members;
    STAILQ(geo_t) members;	/*!< \brief All geo_t members in this coord. */

    subject_t *mouse_event;
};
/*! \defgroup coord_flags Coord Flags
 * @{
 */
#define COF_DIRTY 0x1
#define COF_HIDDEN 0x2	        /*!< A coord is hidden. */
#define COF_OWN_CANVAS 0x4	/*!< A coord owns a canvas or inherit it
				 * from an ancestor. 
				 */
#define COF_SKIP_TRIVAL 0x8	/*!< temporary skip descendants
				 * when trivaling.
				 */
#define COF_FREE 0x10
#define COF_FAST_CACHE 0x20	/*!< \brief Cache raster image in fast way.
				 * \sa \ref img_cache
				 */
#define COF_PRECISE_CACHE 0x40	/*!< \brief Cache raster image in
				 * precise way.
				 * \sa \ref img_cache
				 */
#define COF_CACHE_MASK 0x60
#define COF_ANCESTOR_CACHE 0x80	/*!< \brief One ancestor is cached.
				 * \sa \ref img_cache
				 */
#define COF_MUST_ZEROING 0x100	/*!< \sa \ref cache_imp */
#define COF_JUST_CLEAN 0x200	/*!< \brief This coord is just cleaned by
				 *    last clean.
				 * It is used by clean_rdman_dirties().
				 */
#define COF_TEMP_MARK 0x400	/*!< \brief Temporary mark a coord. */
/* @} */

extern void matrix_mul(co_aix *m1, co_aix *m2, co_aix *dst);
extern void matrix_trans_pos(co_aix *matrix, co_aix *x, co_aix *y);

extern void coord_init(coord_t *co, coord_t *parent);
extern void coord_trans_pos(coord_t *co, co_aix *x, co_aix *y);
extern co_aix coord_trans_size(coord_t *co, co_aix size);
extern void compute_aggr_of_coord(coord_t *coord);
extern void compute_aggr_of_cached_coord(coord_t *coord);
extern void compute_reverse(co_aix *orig, co_aix *reverse);
extern void update_aggr_matrix(coord_t *start);
extern coord_t *preorder_coord_subtree(coord_t *root, coord_t *last);
extern coord_t *postorder_coord_subtree(coord_t *root, coord_t *last);
#define preorder_coord_skip_subtree(sub)		\
    do { (sub)->flags |= COF_SKIP_TRIVAL; } while(0)
#define coord_hide(co)		      \
    do {			      \
	(co)->flags |= COF_HIDDEN;    \
    } while(0)
#define coord_show(co) do { co->flags &= ~COF_HIDDEN; } while(0)
#define coord_fast_cache(co)					\
    do {							\
	(co)->flags =						\
	    ((co)->flags & ~COF_CACHE_MASK) | COF_FAST_CACHE;	\
    } while(0)
#define coord_precise_cache(co)						\
    do {								\
	(co)->flags =							\
	    ((co)->flags & ~COF_CACHE_MASK) | COF_PRECISE_CACHE;	\
    } while(0)
#define coord_nocache(co)					\
    do {							\
	(co)->flags &= ~COF_CACHE_MASK;				\
    } while(0)
#define coord_is_root(co) ((co)->parent == NULL)
#define coord_is_cached(co) ((co)->flags & COF_CACHE_MASK)
#define coord_is_fast_cached(co) ((co)->flags & COF_FAST_MASK)
#define coord_is_precise_cached(co) ((co)->flags & COF_PRECISE_MASK)
#define coord_is_zeroing(co) ((co)->flags & COF_MUST_ZEROING)
#define coord_set_zeroing(co) \
    do { (co)->flags |= COF_MUST_ZEROING; } while(0)
#define coord_clear_zeroing(co) \
    do { (co)->flags &= ~COF_MUST_ZEROING; } while(0)
#define coord_set_flags(co, _flags)		\
    do { (co)->flags |= (_flags); } while(0)
#define coord_get_flags(co, _flags) ((co)->flags & (_flags))
#define coord_clear_flags(co, _flags)		\
    do { (co)->flags &= ~(_flags); } while(0)
#define coord_get_mouse_event(coord) ((coord)->mouse_event)
#define coord_get_aggr_matrix(coord) ((coord)->aggr_matrix)
#define FOR_COORDS_POSTORDER(coord, cur)			\
    for((cur) = postorder_coord_subtree((coord), NULL);		\
	(cur) != NULL;						\
	(cur) = postorder_coord_subtree((coord), (cur)))
#define FOR_COORDS_PREORDER(coord, cur)			\
    for((cur) = (coord);				\
	(cur) != NULL;					\
	(cur) = preorder_coord_subtree((coord), (cur)))

/*! \brief Coord operation function
 * These functions are used to move and scale the coord_t. Programmers should use these functions instead of using the matrix directly.
 * The x,y,sx,sy are all in co_aix type.
 *
 */
#define coord_move(co,x,y) do {(co)->matrix[2] = (x); (co)->matrix[5] = (y);} while(0)
#define coord_set_scalex(co,sx) do {(co)->matrix[0] = sx;} while(0)
#define coord_set_scaley(co,sy) do {(co)->matrix[3] = sy;} while(0)
#define coord_scalex(co) ((co)->matrix[0])
#define coord_scaley(co) ((co)->matrix[3])
#define coord_x(co) ((co)->matrix[2])
#define coord_y(co) ((co)->matrix[5])
#define FOR_COORD_MEMBERS(coord, geo)			\
    for(geo = STAILQ_HEAD((coord)->members);		\
	geo != NULL;					\
	geo = STAILQ_NEXT(geo_t, coord_next, geo))
#define FOR_COORD_SHAPES(coord, shape)			\
    for(shape = geo_get_shape_safe(STAILQ_HEAD((coord)->members));	\
	shape != NULL;							\
	shape = geo_get_shape_safe(STAILQ_NEXT(geo_t, coord_next,	\
					       sh_get_geo(shape))))
#define coord_get_area(coord) ((coord)->cur_area)
#define _coord_get_canvas(coord) ((coord)->canvas_info->canvas)
#define _coord_set_canvas(coord, _canvas)		\
    do {						\
	(coord)->canvas_info->canvas = _canvas;		\
    } while(0)
#define _coord_get_dirty_areas(coord) (&(coord)->canvas_info->dirty_areas)

/* @} */

/*! \brief A grahpic shape.
 *
 * \dot
 * digraph G {
 * "shape" -> "coord";
 * "shape" -> "geo";
 * "geo" -> "shape";
 * "coord" -> "shape" [label="members"]
 * "shape" -> "shape" [label="sibling"];
 * }
 * \enddot
 */
struct _shape {
    mb_obj_t obj;
    geo_t *geo;
    coord_t *coord;
    paint_t *fill, *stroke;
    co_aix stroke_width;
    int stroke_linecap:2;
    int stroke_linejoin:2;
    struct _shape *sh_next;	/*!< Link all shapes of a rdman together. */
    void (*free)(shape_t *shape);
};
/* enum { SHT_UNKNOW, SHT_PATH, SHT_TEXT, SHT_RECT }; */

#define sh_get_mouse_event_subject(sh) ((sh)->geo->mouse_event)
#define sh_hide(sh)			     \
    do {				     \
	(sh)->geo->flags |= GEF_HIDDEN;	     \
    } while(0)
#define sh_show(sh)					\
    do {						\
	(sh)->geo->flags &= ~GEF_HIDDEN;		\
    } while(0)
#define sh_get_geo(sh) ((sh)->geo)
#define sh_get_geo_safe(sh) ((sh)? (sh)->geo: NULL)
#define sh_get_flags(sh, mask) geo_get_flags(sh_get_geo(sh), mask)
#define sh_set_flags(sh, mask) geo_set_flags(sh_get_geo(sh), mask)
#define sh_clear_flags(sh, mask) geo_clear_flags(sh_get_geo(sh), mask)
#define sh_pos_is_in(sh, x, y) geo_pos_is_in(sh_get_geo(sh), x, y)
#define sh_get_area(sh) geo_get_area(sh_get_geo(sh))
#define sh_get_coord(sh) ((sh)->coord)
#define sh_get_aggr_matrix(sh) (coord_get_aggr_matrix(sh_get_coord(sh)))
#define sh_get_fill(sh) ((sh)->fill)
#define sh_get_stroke(sh) ((sh)->stroke)


/*! \brief A sprite is a set of graphics that being an object in animation.
 *
 * A sprite include graphics comprise an object.  For example, a tank, in
 * example tank, is comprised a set of graphics that is represented as a
 * sprite.
 */
struct _mb_sprite {
    void (*free)(mb_sprite_t *);
    mb_obj_t *(*get_obj_with_name)(mb_sprite_t *sprite, const char *id);
    /*! Return non-zero for error. */
    int (*goto_scene)(mb_sprite_t *sprite, int scene_no);
};

#define MB_SPRITE_FREE(sprite) ((mb_sprite_t *)(sprite))->free(sprite)
#define MB_SPRITE_GET_OBJ(sprite, name)					\
    ((mb_sprite_t *)(sprite))->get_obj_with_name((mb_sprite_t *)(sprite), \
						 (name))
#define MB_SPRITE_GOTO_SCENE(sprite, scene_no)				\
    ((mb_sprite_t *)(sprite))->goto_scene((mb_sprite_t *)(sprite), scene_no)


/*! \defgroup mb_sprite_lsym Sprite with linear symbol table.
 * @{
 */ 
struct _mb_sprite_lsym_entry {
    const char *sym;
    const int offset;
};
typedef struct _mb_sprite_lsym_entry mb_sprite_lsym_entry_t;

/*! \brief A sub-type of mb_sprite_t with linear symbol table.
 *
 * This type of sprite search symbols with linear/or binary searching.
 */
struct _mb_sprite_lsym {
    mb_sprite_t sprite;
    int num_entries;
    mb_sprite_lsym_entry_t *entries;
};
typedef struct _mb_sprite_lsym mb_sprite_lsym_t;

/* @} */

#endif /* __MB_TYPES_H_ */
