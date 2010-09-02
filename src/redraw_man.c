#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "mb_graph_engine.h"
#include "mb_types.h"
#include "mb_shapes.h"
#include "mb_tools.h"
#include "mb_redraw_man.h"
#include "mb_observer.h"
#include "mb_prop.h"
#include "config.h"

/* required by rdman_img_ldr_load_paint() */
#include "mb_paint.h"

/*! \page dirty Dirty geo, coord, and area.
 *
 * \section dirty_of_ego Dirty of geo
 * A geo is dirty when any of the shape, size or positions is changed.
 * It's geo and positions should be recomputed before drawing.  So,
 * dirty geos are marked as dirty and put into redraw_man_t::dirty_geos list.
 * geos in the list are cleaned to compute information as a reaction for
 * dirty.  It recomputes size, position and other data of
 * repective shapes.
 *
 * \section dirty_of_coord Dirty of coord
 * A coord is dirty when it's transformation matrix being changed.
 * Dirty coords are marked as dirty and put into dirty_coords list.
 * Once a coord is dirty, every member geos of it are also dirty.
 * Because, their shape, size and positions will be changed.  But,
 * they are not marked as dirty and put into dirty_geos list, since
 * all these member geos will be recomputed for computing new current
 * area of the coord.  The changes of a coord also affect child
 * coords.  Once parent is dirty, all children are also dirty for
 * their aggregate matrix out of date.  Dirty coords should be
 * clean in preorder of tree traversal.  The redraw_man_t::dirty_coords
 * list are sorted to keep ordering before cleaning.
 * Whenever a coord is marked dirty and put into redraw_man_t::dirty_coords
 * list, all it's children should also be marked.
 *
 * The procedure of clean coords comprises recomputing aggregated
 * transform matrix and area where members spreading in.  The aggregated
 * transform matrix can reduce number of matrix mul to transform
 * positions from space of a coord to the closest cached ancestor coord.
 *
 * The list is inspected before drawing to recompute new shape, size,
 * and positions of member geos of coords in the list.  The drity
 * flag of member geos will be clean.
 *
 * Clean coords should be performed before clean geos, since clean
 * coords will also clean member geos.
 *
 * \section dirty_of_area Dirty of area
 * When an area is dirty, it is added to coord_canvas_info_t::dirty_areas
 * of it's closest cached coord.  Areas are created when a shape is cleaned
 * for dirty.  The areas where a cleaned shape occupied before and after
 * cleaning should be redrawed.  Areas are added to dirty area list to
 * mark areas where should be redrawed.  So, all shapes covered by
 * dirty area list should be redrawed to update these areas.  So, areas
 * are added to dirty lists after cleaning geos due to changes of
 * shapes.
 *
 * For example, when a shape is moved from location A to location B,
 * areas where the shape occupied for A and B are changed for moving.
 * Bothe areas are added into dirty list to mark these areas should
 * be redrawed.
 */

/*! \page redraw How to Redraw Shapes?
 *
 * Coords are corresponding objects for group tags of SVG files.
 * In conceptional, every SVG group has a canvas, graphics of child shapes
 * are drawed into the canvas, applied filters of group, and blended into
 * canvas of parent of the group.
 *
 * But, we don't need to create actually a surface/canvas for every coord.
 * We only create surface for coords their opacity value are not 1 or they
 * apply filters on background.  Child shapes of coords without canvas
 * are drawed on canvas of nearest ancestor which have canvas.  It said
 * a coord owns a canvas or inherits from an ancestor. (\ref COF_OWN_CANVAS,
 * clean_coord()) Except, root_coord always owns a canvas.
 *
 * \note Default opacity of a coord is 1.
 *
 * \sa
 * - rdman_redraw_all()
 * - rdman_redraw_changed()
 * - draw_shapes_in_areas()
 *
 * \section img_cache Image Cache
 * It costs time to redraw every component in a complete graphic.
 * Image cache try to cache result of prviously rendering, and reusing it
 * to avoid wasting CPU time on repeatitive and redundant rendering.
 *
 * \ref COF_FAST_CACHE and \ref COF_PRECISE_CACHE are used to tag a
 * coord that it's
 * rendering result is cached in fast way or precise way.  With fast cache,
 * MB renders descendants of a coord in once, and reuse the result until it
 * being dirty.  With precise cache, it alike fast cache, but it also
 * performs rendering when an ancester of the coord transform it to larger
 * size, in width or height.
 *
 * coord_t::aggr_matrix of a cached coord is computed from aggr_matrix of
 * parent.  But, it does not use one from parent directly.  parent one is
 * transformed as
 * \code
 * cache_scale_x = sqrt(p_matrix[0]**2 + p_matrix[3]**2);
 * cache_scale_y = sqrt(p_matrix[1]**2 + p_matrix[4]**2);
 * cache_p_matrix[0] = cache_scale_x;
 * cache_p_matrix[1] = 0;
 * cache_p_matrix[2] = range_shift_x;
 * cache_p_matrix[3] = 0;
 * cache_p_matrix[4] = cache_scale_y;
 * cache_p_matrix[5] = range_shift_y;
 * \endcode
 * where p_matrix is parent one, and cache_p_matrix is one derived from
 * parent one.  coord_t::aggr_matrix of a cached coord is
 * \code
 * aggr_matrix = cache_p_matrix * matrix
 * \endcode
 * where matrix is the transform being installed on the cached coord.
 * range_shift_x and range_shift_y are defined above.
 *
 * cache_p_matrix rescales sub-graphic to an appropriately size
 * (cache_scale_x, cache_scale_y) and aligns left-top of the minimum
 * rectangle (range_shift_x, range_shift_y) that cover the area occupied
 * by sub-graphic with origin of the space.
 *
 * The sub-graphic should be rendered on space defined by cache_p_matrix of
 * cached one.  But rendering result are transformed to the space defined
 * by parent with following matrix.
 * \code
 * draw_matrix = reverse(p_matrix * reverse(cache_p_matrix))
 * \endcode
 * With Cairo, draw_matrix is applied on source surface (canvas)
 * to draw image to parent's surface (canvas).  draw_matrix is a function
 * map points from parent space to the space of cached one.
 *
 * Cached coords are marked for changing transformation of ancestors only if
 * following condition is true.
 * \code
 * cache_scale_x < sqrt(p_matrix[0]**2 + p_matrix[3]**2) ||
 * cache_scale_y < sqrt(p_matrix[1]**2 + p_matrix[4]**2)
 * \endcode
 * where p_matrix is latest aggr_matrix of parent after changing
 * transformation, and where cache_scale_* are ones mention above and computed
 * before changing transformation of ancestors.
 *
 * Cache_scale_* can be recovered by following instructions.
 * \code
 * cache_scale_x = aggr_matrix[0] / matrix[0];
 * cache_scale_y = aggr_matrix[4] / matrix[4];
 * \endcode
 *
 * \section cache_area Area of cached coord
 * - *_transform of shapes works as normal
 *   - areas of descendants of cached coord are in space defined
 *     by aggr_matrix of cached coord.
 *   - descendants are marked with \ref COF_ANCESTOR_CACHE
 * 
 * Since *_transform of shapes compute area with aggr_matrix that is
 * derived from aggr_matrix of a cached ancestor, area of
 * \ref COF_ANCESTOR_CACHE ones should be transformed to device space in
 * find_shape_at_pos() with following statement.
 * \code
 * area_matrix = p_matrix * reverse(cache_p_matrix)
 * \endcode
 * where cache_p_matrix and p_matrix are corresponding matrix of
 * cached ancestor.  We can also perform transforming in reversed
 * direction to transform point to space defined by aggr_matrix of cached
 * coord.
 *
 * Since it is costly to transform area of \ref COF_ANCESTOR_CACHE ones to
 * device space if more than one ancestor are cached, no ancestor of
 * cached coord can be set to cached.
 *
 * \section cached_bounding Bounding box of cached coord and descendants
 * Bounding box of a cached coord and it's descendants is the range that
 * cached coord and descendants are rendered on canvas.  It is also called
 * cached-bounding.
 *
 * range_shift_x and range_shift_y are computed by initailizing cache_p_matrix
 * with range_shift_x == range_shift_y == 0 at first.  cache_p_matrix is
 * used to compute aggr_matrix and cached-bounding in turn.  Then,
 * range_shift_x and range_shift_y are initialized to negative of
 * x-axis and y-axis, repectively, of left-top of cached-bounding.  Then,
 * aggr_matrix of cached coord and descendants are updated by
 * following statements.
 * \code
 * aggr_matrix[2] += range_shift_x;
 * aggr_matrix[5] += range_shift_y;
 * \endcode
 * The statements shift the spaces to make cached-bounding
 * aligned to origin of coordinate system.
 * The purpose of range_shift_* is to reduce size of canvas used to cache
 * rendering result.  The canvas are shrink to size the same as bounding
 * box.
 *
 * \section cache_redraw How cache and redraw work together?
 * When a coord and descedants are cached, the coord is flaged with
 * COF_FAST_CACHE or COF_PRECISE_CACHE.  When a coord is marked dirty, all
 * descendants are also marked dirty by rdman except descendants of cached
 * ones.  But, cached ones are also marked dirty as normal ones.  The
 * reason to mark cached ones is giving them a chance to update their
 * area.
 *
 * For precise cached descendants, above rule has an exception.  They should
 * also be marked dirty if cached coord should be rendered in a larger
 * resize factor to get better output.
 *
 * coord_t::aggr_matrix and cached-bounding of cached coord must be computed
 * in the way described in \ref cached_bounding.  Propagating range_shift_*
 * to descendants must skip cached ones and their descendants.
 * Range_shift_* are computed after updating descendants.  So, procedure
 * of clean descendants of a cached one must performed in two phases.
 * One for computing areas of descendants and one for propagating
 * range_shift_*.
 *
 * A cached coord or/and descendants are dirty only for cached coord or
 * descendants being marked dirty by application.  Once a cached coord or
 * descendant is marked dirty, all descendants of marked one are also
 * marked.  redraw_man_t::dirty_areas collects areas, in device space,
 * that should be updated.  All shapes overlaid with any area in
 * redraw_man_t::dirty_areas should be redraw.  Since descendants of cached
 * coord compute their areas in spaces other than device space.
 * Separated lists should be maintained for each cached coord and it's
 * descendants.
 *
 * \section cache_imp Implementation of Cache
 * Both cached coords and coords that opacity != 1 need a canvas to
 * draw descendants on.  Both cases are traded in the same way.  Every
 * of them own a canvas_info to describe canvas and related
 * information.  aggr_matrix of descendants must be adjusted to make
 * left-top of bounding box just at origin (0, 0) of canvas.  It saves
 * space to give a canvas just enough for rending descadants.  The
 * process of adjusting left-top of bounding box is zeroing.
 *
 * Following is rules.
 * - zeroing on a cached coord is performed by adjust coord_t::aggr_matrix 
 *   of the cached coord and descendnats.
 * - Clean coords works just like before without change.
 *   - in preorder
 * - never perform zeroing on root_coord.
 * - do zeroing on cached coords marked with \ref COF_MUST_ZEROING.
 *   - when clean a descendant that moves out-side of it's canvas,
 *     respective cached coord is marked with \ref COF_MUST_ZEROING.
 *   - zeroing is performed immediately after clean coords.
 *   - zeroing will not be propagated to ancestors of a cached coord.
 *     - It will be stopped once a cached coord being found.
 *     - coord_t::cur_area and coord_t::aggr_matrix of cached coords
 *       must be ajdusted.
 * - the area of a cached coord is defined in parent space.
 *   - areas of descendants are defined in space defined by aggr_matrix of
 *     cached coord.
 *     - coord_t::aggr_matrix of cached coord defines coordination of
 *       descendants.
 *   - the parent knows the area in where cached coord and descendnats will
 *     be draw.
 * - cached coords keep their private dirty area list.
 *   - private dirty areas of a cached coord are transformed and added to
 *     parent cached coord.
 *   - aggregates areas before adding to parent.
 * - canvas of a cached coord is updated if
 *   - descendants are dirty, or
 *   - it-self is dirty.
 * - change of a canvas must copy to canvas of parent space.
 *   - a cached is updated if canvas of descendant cached coord is updated.
 * - updating canvas is performed by redraw dirty areas.
 *   - since dirty areas of cached ones would be aggregated and added to
 *     parent, parent cached coord would copy it from cache of descedants.
 * - descendant cached coords must be updated before ancestor cached coords.
 *   - add dirty areas to parent immediately after updating canvas.
 * - Making dirty coords is not propagated through cached ones.
 *   - cached ones are also made dirty, but stop after that.
 *
 * Steps:
 * - SWAP coord_t::cur_area of dirty coords.
 * - SWAP geo_t::cur_area of dirty geos.
 * - clean coords
 *   - coord_t::aggr_matrix of cached coord is not the same as non-cached.
 *   - see \ref img_cache
 * - clean geos
 * - Add canvas owner of dirty geos to redraw_man_t::zeroing_coords
 *   - Cached ancestors of redraw_man_t::dirty_geos
 *   - Cached ancestors of redraw_man_t::dirty_coords
 *   - Cached ancestors of zeroed ones should also be zeroed.
 * - zeroing
 *   - Add more dirty areas if canvas should be fully redrawed.
 *   - From leaves to root.
 *   - Adjust area of child cached coords.
 * - add aggregated dirty areas from descendant cached coords to ancestors.
 *   - Must include old area of cached coords if it is just clean and
 *     parent cached one is not just clean.
 *   - Just clean is a coord cleaned in last time of cleaning coords.
 * - draw dirty areas
 *   - areas are rounded to N at first.
 *   - from leaves to root.
 */

#ifndef ASSERT
#define ASSERT(x)
#endif

/* NOTE: bounding box should also consider width of stroke.
 */

#define sh_attach_geo(sh, g)			\
    do {					\
	(sh)->geo = g;				\
	(g)->shape = (shape_t *)(sh);		\
    } while(0)
#define sh_detach_geo(sh)			\
    do {					\
	(sh)->geo->shape = NULL;		\
	(sh)->geo = NULL;			\
    } while(0)
#define sh_get_geo(sh) ((sh)->geo)
#define sh_attach_coord(sh, coord) do { (sh)->coord = coord; } while(0)
#define sh_detach_coord(sh) do { (sh)->coord = NULL; } while(0)
#define rdman_is_dirty(rdman)			\
    ((rdman)->dirty_coords.num != 0 ||		\
     (rdman)->dirty_geos.num != 0)

#define OK 0
#define ERR -1

#define ARRAY_EXT_SZ 64

#define SWAP(a, b, t) do { t c;  c = a; a = b; b = c; } while(0)

#ifdef UNITTEST
typedef struct _sh_dummy sh_dummy_t;

extern void sh_dummy_transform(shape_t *shape);
extern void sh_dummy_fill(shape_t *, mbe_t *);
#endif /* UNITTEST */

static subject_t *ob_subject_alloc(ob_factory_t *factory);
static void ob_subject_free(ob_factory_t *factory, subject_t *subject);
static observer_t *ob_observer_alloc(ob_factory_t *factory);
static void ob_observer_free(ob_factory_t *factory, observer_t *observer);
static subject_t *ob_get_parent_subject(ob_factory_t *factory,
					subject_t *cur_subject);
/* Functions for children. */
#define FORCHILDREN(coord, child)				\
    for((child) = STAILQ_HEAD((coord)->children);		\
	(child) != NULL;					\
	(child) = STAILQ_NEXT(coord_t, sibling, (child)))
#define NEXT_CHILD(child) STAILQ_NEXT(coord_t, sibling, child)
#define ADD_CHILD(parent, child)					\
    STAILQ_INS_TAIL((parent)->children, coord_t, sibling, (child))
#define RM_CHILD(parent, child)						\
    STAILQ_REMOVE((parent)->children, coord_t, sibling, (child))
#define FIRST_CHILD(parent) STAILQ_HEAD((parent)->children)

/* Functions for members. */
#define FORMEMBERS(coord, member)				\
    for((member) = STAILQ_HEAD((coord)->members);		\
	(member) != NULL;					\
	(member) = STAILQ_NEXT(geo_t, coord_next, (member)))
#define NEXT_MEMBER(member) STAILQ_NEXT(geo_t, coord_next, (member))
#define ADD_MEMBER(coord, member)					\
    STAILQ_INS_TAIL((coord)->members, geo_t, coord_next, (member))
#define RM_MEMBER(coord, member)					\
    STAILQ_REMOVE((coord)->members, geo_t, coord_next, (member))
#define FIRST_MEMBER(coord) STAILQ_HEAD((coord)->members)

/* Functions for paint members. */
#define FORPAINTMEMBERS(paint, member)			\
    for((member) = STAILQ_HEAD((paint)->members);	\
	(member) != NULL;				\
	(member) = STAILQ_NEXT(paint_t, next, member))
#define RM_PAINTMEMBER(paint, member)				\
    STAILQ_REMOVE((paint)->members, shnode_t, next, member)

/*! \brief Sort a list of element by a unsigned integer.
 *
 * The result is in ascend order.  The unsigned integers is
 * at offset specified by 'off' from start address of elemnts.
 */
static void _insert_sort(void **elms, int num, int off) {
    int i, j;
    unsigned int val;
    void *elm_i;

    for(i = 1; i < num; i++) {
	elm_i = elms[i];
	val = *(unsigned int *)(elm_i + off);
	for(j = i; j > 0; j--) {
	    if(*(unsigned int *)(elms[j - 1] + off) <= val)
		break;
	    elms[j] = elms[j - 1];
	}
	elms[j] = elm_i;
    }
}

DARRAY_DEFINE(coords, coord_t *);
DARRAY_DEFINE(geos, geo_t *);
DARRAY_DEFINE(areas, area_t *);

int rdman_add_gen_geos(redraw_man_t *rdman, geo_t *geo) {
    int r;

    r = geos_add(rdman_get_gen_geos(rdman), geo);
    return r;
}

/*! Use \brief DARRAY to implement dirty & free lists.
 */
#define ADD_DATA(sttype, field, v)		\
    int r;					\
    r = sttype ## _add(&rdman->field, v);	\
    return r == 0? OK: ERR;


static int is_area_in_areas(area_t *area,
			     int n_areas,
			     area_t **areas) {
    int i;

    for(i = 0; i < n_areas; i++) {
	if(areas_are_overlay(area, areas[i]))
	    return 1;
    }
    return 0;
}

static int is_geo_in_areas(geo_t *geo,
			     int n_areas,
			     area_t **areas) {
    return is_area_in_areas(geo->cur_area, n_areas, areas);
}

static void area_to_positions(area_t *area, co_aix (*poses)[2]) {
    poses[0][0] = area->x;
    poses[0][1] = area->y;
    poses[1][0] = area->x + area->w;
    poses[1][1] = area->y + area->h;;
}

/* Maintain Lists */

static int add_dirty_coord(redraw_man_t *rdman, coord_t *coord) {
    coord->flags |= COF_DIRTY;
    ADD_DATA(coords, dirty_coords, coord);
    return OK;
}

static int add_dirty_geo(redraw_man_t *rdman, geo_t *geo) {
    geo->flags |= GEF_DIRTY;
    ADD_DATA(geos, dirty_geos, geo);
    return OK;
}

static int add_dirty_area(redraw_man_t *rdman, coord_t *coord, area_t *area) {
    int r;
    
    if(area->w < 0.01 || area->h < 0.01)
	return OK;
    
    rdman->n_dirty_areas++;
    r = areas_add(_coord_get_dirty_areas(coord), area);
    return r == 0? OK: ERR;
}

static int add_zeroing_coord(redraw_man_t *rdman, coord_t *coord) {
    coord_set_zeroing(coord);
    ADD_DATA(coords, zeroing_coords, coord);
    return OK;
}

static int add_dirty_pcache_area_coord(redraw_man_t *rdman, coord_t *coord) {
    coord_set_flags(coord, COF_DIRTY_PCACHE_AREA);
    ADD_DATA(coords, dirty_pcache_area_coords, coord);
    return OK;
}

static int add_free_obj(redraw_man_t *rdman, void *obj,
			free_func_t free_func) {
    int max;
    free_obj_t *new_objs, *free_obj;

    if(rdman->free_objs.num >= rdman->free_objs.max) {
	max = rdman->free_objs.num + ARRAY_EXT_SZ;
	new_objs = realloc(rdman->free_objs.objs,
				max * sizeof(free_obj_t));
	if(new_objs == NULL)
	    return ERR;
	rdman->free_objs.max = max;
	rdman->free_objs.objs = new_objs;
    }

    free_obj = rdman->free_objs.objs + rdman->free_objs.num++;
    free_obj->obj = obj;
    free_obj->free_func = free_func;

    return OK;
}

static void free_free_objs(redraw_man_t *rdman) {
    int i;
    free_obj_t *free_obj;

    for(i = 0; i < rdman->free_objs.num; i++) {
	free_obj = &rdman->free_objs.objs[i];
	free_obj->free_func(rdman, free_obj->obj);
    }
    rdman->free_objs.num = 0;
}

static void free_objs_destroy(redraw_man_t *rdman) {
    if(rdman->free_objs.objs != NULL)
	free(rdman->free_objs.objs);
}



static mbe_t *canvas_new(int w, int h) {
#ifndef UNITTEST
    mbe_surface_t *surface;
    mbe_t *cr;
    
    surface = mbe_image_surface_create(MB_IFMT_ARGB32,
					 w, h);
    cr = mbe_create(surface);

    return cr;
#else
    return NULL;
#endif
}

static void canvas_free(mbe_t *canvas) {
#ifndef UNITTEST
    mbe_destroy(canvas);
#endif
}

static void canvas_get_size(mbe_t *canvas, int *w, int *h) {
#ifndef UNITTEST
    mbe_surface_t *surface;

    surface = mbe_get_target(canvas);
    *w = mbe_image_surface_get_width(surface);
    *h = mbe_image_surface_get_height(surface);
#else
    *w = 0;
    *h = 0;
#endif
}

static int geo_off_in_coord(geo_t *geo, coord_t *coord) {
    int off = 0;
    geo_t *vgeo;

    FORMEMBERS(coord, vgeo) {
	if(vgeo == geo)
	    return off;
	off++;
    }
    return -1;
}

static void geo_attach_coord(geo_t *geo, coord_t *coord) {
    ADD_MEMBER(coord, geo);
    coord->num_members++;
}

static void geo_detach_coord(geo_t *geo, coord_t *coord) {
    int off;
    coord_t *child;

    off = geo_off_in_coord(geo, coord);
    if(off < 0)
	return;
    FORCHILDREN(coord, child) {
	if(child->before_pmem >= off)
	    child->before_pmem--;
    }

    RM_MEMBER(coord, geo);
    coord->num_members--;
}

static coord_canvas_info_t *coord_canvas_info_new(redraw_man_t *rdman,
						  coord_t *coord,
						  mbe_t *canvas) {
    coord_canvas_info_t *info;

    info = (coord_canvas_info_t *)elmpool_elm_alloc(rdman->coord_canvas_pool);
    if(info == NULL)
	return info;
    
    info->owner = coord;
    info->canvas = canvas;
    DARRAY_INIT(&info->dirty_areas);
    
    bzero(info->pcache_areas, sizeof(area_t) * 2);
    info->pcache_cur_area = &info->pcache_areas[0];
    info->pcache_last_area = &info->pcache_areas[1];

    return info;
}

static void coord_canvas_info_free(redraw_man_t *rdman,
				   coord_canvas_info_t *info) {
    DARRAY_DESTROY(&info->dirty_areas);
    elmpool_elm_free(rdman->coord_canvas_pool, info);
}

static void mouse_event_root_dummy(event_t *evt, void *arg) {
}

int redraw_man_init(redraw_man_t *rdman, mbe_t *cr, mbe_t *backend) {
    extern void redraw_man_destroy(redraw_man_t *rdman);
    extern int _paint_color_size;
    observer_t *addrm_ob;
    extern void addrm_monitor_hdlr(event_t *evt, void *arg);

    memset(rdman, 0, sizeof(redraw_man_t));

    DARRAY_INIT(&rdman->dirty_coords);
    DARRAY_INIT(&rdman->dirty_pcache_area_coords);
    DARRAY_INIT(&rdman->dirty_geos);
    DARRAY_INIT(&rdman->gen_geos);
    DARRAY_INIT(&rdman->zeroing_coords);
    
    rdman->geo_pool = elmpool_new(sizeof(geo_t), 128);
    rdman->coord_pool = elmpool_new(sizeof(coord_t), 16);
    rdman->shnode_pool = elmpool_new(sizeof(shnode_t), 16);
    rdman->observer_pool = elmpool_new(sizeof(observer_t), 32);
    rdman->subject_pool = elmpool_new(sizeof(subject_t), 32);
    rdman->paint_color_pool = elmpool_new(_paint_color_size, 64);
    rdman->pent_pool = elmpool_new(sizeof(mb_prop_entry_t), 128);
    rdman->coord_canvas_pool = elmpool_new(sizeof(coord_canvas_info_t), 16);
    if(!(rdman->geo_pool && rdman->coord_pool && rdman->shnode_pool &&
	 rdman->observer_pool && rdman->subject_pool &&
	 rdman->paint_color_pool && rdman->coord_canvas_pool))
	goto err;

    rdman->ob_factory.subject_alloc = ob_subject_alloc;
    rdman->ob_factory.subject_free = ob_subject_free;
    rdman->ob_factory.observer_alloc = ob_observer_alloc;
    rdman->ob_factory.observer_free = ob_observer_free;
    rdman->ob_factory.get_parent_subject = ob_get_parent_subject;

    rdman->redraw =
	subject_new(&rdman->ob_factory, rdman, OBJT_RDMAN);
    rdman->addrm_monitor =
	subject_new(&rdman->ob_factory, rdman, OBJT_RDMAN);
    if(!(rdman->redraw && rdman->addrm_monitor))
	goto err;

    addrm_ob = subject_add_observer(rdman->addrm_monitor,
				    addrm_monitor_hdlr, rdman);
    if(addrm_ob == NULL)
	goto err;

    rdman->last_mouse_over = NULL;

    rdman->root_coord = elmpool_elm_alloc(rdman->coord_pool);
    if(rdman->root_coord == NULL)
	redraw_man_destroy(rdman);
    rdman->n_coords = 1;
    coord_init(rdman->root_coord, NULL);
    mb_prop_store_init(&rdman->root_coord->obj.props, rdman->pent_pool);
    rdman->root_coord->mouse_event = subject_new(&rdman->ob_factory,
						 rdman->root_coord,
						 OBJT_COORD);
    coord_set_flags(rdman->root_coord, COF_OWN_CANVAS);
    rdman->root_coord->canvas_info =
	coord_canvas_info_new(rdman, rdman->root_coord, cr);
    rdman->root_coord->opacity = 1;

    rdman->cr = cr;
    rdman->backend = backend;

    STAILQ_INIT(rdman->shapes);
    
    /* \note To make root coord always have at leat one observer.
     * It triggers mouse interpreter to be installed on root.
     */
    subject_set_monitor(rdman->root_coord->mouse_event,
			rdman->addrm_monitor);
    subject_add_observer(rdman->root_coord->mouse_event,
			 mouse_event_root_dummy, NULL);

    mb_prop_store_init(&rdman->props, rdman->pent_pool);
    return OK;

 err:
    if(rdman->geo_pool)
	elmpool_free(rdman->geo_pool);
    if(rdman->coord_pool)
	elmpool_free(rdman->coord_pool);
    if(rdman->shnode_pool)
	elmpool_free(rdman->shnode_pool);
    if(rdman->observer_pool)
	elmpool_free(rdman->observer_pool);
    if(rdman->subject_pool)
	elmpool_free(rdman->subject_pool);
    if(rdman->paint_color_pool)
	elmpool_free(rdman->paint_color_pool);
    if(rdman->pent_pool)
	elmpool_free(rdman->pent_pool);
    if(rdman->coord_canvas_pool)
	elmpool_free(rdman->coord_canvas_pool);
    DARRAY_DESTROY(&rdman->dirty_coords);
    DARRAY_DESTROY(&rdman->dirty_pcache_area_coords);
    DARRAY_DESTROY(&rdman->dirty_geos);
    DARRAY_DESTROY(&rdman->gen_geos);
    DARRAY_DESTROY(&rdman->zeroing_coords);
    return ERR;
}

void redraw_man_destroy(redraw_man_t *rdman) {
    coord_t *coord, *saved_coord;
    shape_t *shape;
    geo_t *member;

    mb_prop_store_destroy(&rdman->props);

    free_free_objs(rdman);
    free_objs_destroy(rdman);

    /* Mark rdman clean that shapes and coords can be freed
     *  successfully.
     */
    DARRAY_CLEAN(&rdman->dirty_coords);
    DARRAY_CLEAN(&rdman->dirty_pcache_area_coords);
    DARRAY_CLEAN(&rdman->dirty_geos);

    coord = postorder_coord_subtree(rdman->root_coord, NULL);
    while(coord) {
	saved_coord = coord;
	coord = postorder_coord_subtree(rdman->root_coord, coord);
	FORMEMBERS(saved_coord, member) {
	    rdman_shape_free(rdman, member->shape);
	}
	rdman_coord_free(rdman, saved_coord);
    }
    /* Resources of root_coord is free by elmpool_free() or
     * caller; for canvas
     */

    while((shape = STAILQ_HEAD(rdman->shapes)) != NULL) {
	rdman_shape_free(rdman, shape);
    }
    
    coord_canvas_info_free(rdman, rdman->root_coord->canvas_info);

    /* XXX: paints are not freed, here.  All resources of paints would
     * be reclaimed by freeing elmpools.
     */
    
    elmpool_free(rdman->coord_pool);
    elmpool_free(rdman->geo_pool);
    elmpool_free(rdman->shnode_pool);
    elmpool_free(rdman->observer_pool);
    elmpool_free(rdman->subject_pool);
    elmpool_free(rdman->paint_color_pool);
    elmpool_free(rdman->pent_pool);
    elmpool_free(rdman->coord_canvas_pool);

    DARRAY_DESTROY(&rdman->dirty_coords);
    DARRAY_DESTROY(&rdman->dirty_pcache_area_coords);
    DARRAY_DESTROY(&rdman->dirty_geos);
    DARRAY_DESTROY(&rdman->gen_geos);
    DARRAY_DESTROY(&rdman->zeroing_coords);
}


#define ASSERT(x)
/*
 * Change transformation matrix
 * - update aggregated transformation matrix
 *   - of coord_t object been changed.
 *   - of children coord_t objects.
 * - redraw members of coord_t objects.
 * - redraw shape objects they are overlaid with members.
 *   - find out overlaid shape objects.
 *   - geo_t of a coord_t object
 *     - can make finding more efficiency.
 *     - fill overlay geo_t objects of members.
 *
 * Change a shape object
 * - redraw changed object.
 * - redraw shape objects they are overlaid with changed object.
 *   - find out overlaid shape objects.
 *
 * That coord and geo of shape objects are setted by user code
 * give user code a chance to collect coord and geo objects together
 * and gain interest of higher cache hit rate.
 */

int rdman_add_shape(redraw_man_t *rdman, shape_t *shape, coord_t *coord) {
    geo_t *geo;
    int r;

    geo = elmpool_elm_alloc(rdman->geo_pool);
    if(geo == NULL)
	return ERR;
    
    geo_init(geo);
    geo->mouse_event = subject_new(&rdman->ob_factory, geo, OBJT_GEO);
    subject_set_monitor(geo->mouse_event, rdman->addrm_monitor);

    geo_attach_coord(geo, coord);

    /* New one should be dirty to recompute it when drawing. */
    r = add_dirty_geo(rdman, geo);
    if(r != OK)
	return ERR;

    sh_attach_coord(shape, coord);
    sh_attach_geo(shape, geo);

    return OK;
}

/*! \brief Remove a shape object from redraw manager.
 *
 * \note Shapes should be removed after redrawing or when rdman is in clean.
 * \note Removing shapes or coords when a rdman is dirty, removing
 *       is postponsed.
 * \todo redraw shape objects that overlaid with removed one.
 */
int rdman_shape_free(redraw_man_t *rdman, shape_t *shape) {
    geo_t *geo;
    int r;

    geo = shape->geo;

    if(rdman_is_dirty(rdman) && geo != NULL) {
	if(geo->flags & GEF_FREE)
	    return ERR;

	geo->flags |= GEF_FREE;
	sh_hide(shape);
	if(!(geo->flags & GEF_DIRTY)) {
	    r = add_dirty_geo(rdman, geo);
	    if(r != OK)
		return ERR;
	}
	r = add_free_obj(rdman, shape, (free_func_t)rdman_shape_free);
	if(r != OK)
	    return ERR;
	return OK;
    }

    if(shape->stroke != NULL)
	rdman_paint_stroke(rdman, (paint_t *)NULL, shape);
    if(shape->fill != NULL)
	rdman_paint_fill(rdman, (paint_t *)NULL, shape);
    
    if(geo != NULL) {
	subject_free(geo->mouse_event);
	geo_detach_coord(geo, shape->coord);
	sh_detach_coord(shape);
	sh_detach_geo(shape);
	elmpool_elm_free(rdman->geo_pool, geo);
    }
    STAILQ_REMOVE(rdman->shapes, shape_t, sh_next, shape);
    mb_prop_store_destroy(&shape->obj.props);
    shape->free(shape);

    if(rdman->last_mouse_over == (mb_obj_t *)shape)
	rdman->last_mouse_over = NULL;

    
    return OK;
}

shnode_t *shnode_new(redraw_man_t *rdman, shape_t *shape) {
    shnode_t *node;

    node = (shnode_t *)elmpool_elm_alloc(rdman->shnode_pool);
    if(node) {
	node->shape = shape;
	node->next = NULL;
    }
    return node;
}

int rdman_paint_free(redraw_man_t *rdman, paint_t *paint) {
    shnode_t *shnode, *saved_shnode;
    shape_t *shape;

    if(rdman_is_dirty(rdman)) {
	if(paint->flags & PNTF_FREE)
	    return ERR;
	add_free_obj(rdman, paint, (free_func_t)rdman_paint_free);
	paint->flags |= PNTF_FREE;
	return OK;
    }

    /* Free member shapes that using this paint. */
    saved_shnode = NULL;
    FORPAINTMEMBERS(paint, shnode) {
	if(saved_shnode) {
	    RM_PAINTMEMBER(paint, saved_shnode);
	    
	    shape = saved_shnode->shape;
	    if(shape->stroke == paint)
		rdman_paint_stroke(rdman, (paint_t *)NULL, shape);
	    if(shape->fill == paint)
		rdman_paint_fill(rdman, (paint_t *)NULL, shape);
	    
	    shnode_free(rdman, saved_shnode);
	}
	saved_shnode = shnode;
    }
    if(saved_shnode) {
	RM_PAINTMEMBER(paint, saved_shnode);
	
	shape = saved_shnode->shape;
	if(shape->stroke == paint)
	    rdman_paint_stroke(rdman, (paint_t *)NULL, shape);
	if(shape->fill == paint)
	    rdman_paint_fill(rdman, (paint_t *)NULL, shape);
	
	shnode_free(rdman, saved_shnode);
    }

    paint->free(rdman, paint);
    return OK;
}

void _rdman_paint_real_remove_child(redraw_man_t *rdman,
				    paint_t *paint,
				    shape_t *shape) {
    shnode_t *shnode;

    FORPAINTMEMBERS(paint, shnode) {
	if(shnode->shape == shape) {
	    RM_PAINTMEMBER(paint, shnode);
	    shnode_free(rdman, shnode);
	    break;
	}
    }
}

coord_t *rdman_coord_new(redraw_man_t *rdman, coord_t *parent) {
    coord_t *coord, *root_coord;
    coord_t *visit;

    coord = elmpool_elm_alloc(rdman->coord_pool);
    if(coord == NULL)
	return NULL;

    coord_init(coord, parent);
    mb_prop_store_init(&coord->obj.props, rdman->pent_pool);
    coord->mouse_event = subject_new(&rdman->ob_factory,
				     coord,
				     OBJT_COORD);
    subject_set_monitor(coord->mouse_event, rdman->addrm_monitor);
    /*! \note default opacity == 1 */
    coord->opacity = 1;
    if(parent)
	coord->canvas_info = parent->canvas_info;
    rdman->n_coords++;

    coord->order = ++rdman->next_coord_order;
    if(coord->order == 0) {
	rdman->next_coord_order = 0;
	root_coord = visit = rdman->root_coord;
	/* skip root coord. */
	visit = preorder_coord_subtree(root_coord, visit);
	while(visit) {
	    visit->order = ++rdman->next_coord_order;
	    visit = preorder_coord_subtree(root_coord, visit);
	}
    }

    coord->before_pmem = parent->num_members;

    /* If parent is dirty, children should be dirty. */
    if(parent && (parent->flags & COF_DIRTY))
	add_dirty_coord(rdman, coord);

    return coord;
}

static int rdman_coord_free_postponse(redraw_man_t *rdman, coord_t *coord) {
    int r;

    if(coord->flags & COF_FREE)
	return ERR;
    
    coord->flags |= COF_FREE;
    coord_hide(coord);
    if(!(coord->flags & COF_DIRTY)) {
	r = add_dirty_coord(rdman, coord);
	if(r != OK)
	    return ERR;
    }
    r = add_free_obj(rdman, coord, (free_func_t)rdman_coord_free);
    if(r != OK)
	return ERR;
    return OK;
}

/*! \brief Free a coord of a redraw_man_t object.
 *
 * All children and members should be freed before parent being freed.
 *
 * \param coord is a coord_t without children and members.
 * \return 0 for successful, -1 for error.
 *
 * \note Free is postponsed if the coord is dirty or it has children
 *	or members postponsed for free.
 */
int rdman_coord_free(redraw_man_t *rdman, coord_t *coord) {
    coord_t *parent;
    coord_t *child;
    geo_t *member;
    int cm_cnt;			/* children & members counter */

    parent = coord->parent;
    if(parent == NULL)
	return ERR;

    cm_cnt = 0;
    FORCHILDREN(coord, child) {
	cm_cnt++;
	if(!(child->flags & COF_FREE))
	    return ERR;
    }
    FORMEMBERS(coord, member) {
	cm_cnt++;
	if(!(member->flags & GEF_FREE))
	    return ERR;
    }
    
    if(cm_cnt || rdman_is_dirty(rdman))
	return rdman_coord_free_postponse(rdman, coord);

    /* Free canvas and canvas_info (\ref redraw) */
    if(coord_is_cached(coord)) {
	canvas_free(_coord_get_canvas(coord));
	coord_canvas_info_free(rdman, coord->canvas_info);
    }

    RM_CHILD(parent, coord);
    subject_free(coord->mouse_event);
    mb_prop_store_destroy(&coord->obj.props);
    elmpool_elm_free(rdman->coord_pool, coord);
    rdman->n_coords--;

    return OK;
}

static int _rdman_coord_free_members(redraw_man_t *rdman, coord_t *coord) {
    geo_t *member;
    shape_t *shape;
    int r;

    FORMEMBERS(coord, member) {
	shape = geo_get_shape(member);
	r = rdman_shape_free(rdman, shape);
	if(r != OK)
	    return ERR;
    }
    return OK;
}

/*! \brief Free descendant coords and shapes of a coord.
 *
 * The specified coord is also freed.
 */
int rdman_coord_subtree_free(redraw_man_t *rdman, coord_t *subtree) {
    coord_t *coord, *prev_coord;
    int r;

    if(subtree == NULL)
	return OK;

    prev_coord = postorder_coord_subtree(subtree, NULL);
    for(coord = postorder_coord_subtree(subtree, prev_coord);
	coord != NULL;
	coord = postorder_coord_subtree(subtree, coord)) {
	if(!(prev_coord->flags & COF_FREE)) {
	    r = _rdman_coord_free_members(rdman, prev_coord);
	    if(r != OK)
		return ERR;

	    r = rdman_coord_free(rdman, prev_coord);
	    if(r != OK)
		return ERR;
	}
	prev_coord = coord;
    }
    if(!(prev_coord->flags & COF_FREE)) {
	r = _rdman_coord_free_members(rdman, prev_coord);
	if(r != OK)
	    return ERR;

	r = rdman_coord_free(rdman, prev_coord);
	if(r != OK)
	    return ERR;
    }

    return OK;
}

/*! \brief Mark a coord is changed.
 *
 * A changed coord_t object is marked as dirty and put
 * into dirty_coords list.  rdman_coord_changed() should be called
 * for a coord after it been changed to notify redraw manager to
 * redraw shapes grouped by it.
 *
 * Once a coord is changed, all its descendants are also put marked
 * dirty.
 */
int rdman_coord_changed(redraw_man_t *rdman, coord_t *coord) {
    coord_t *child;

    if(coord->flags & COF_DIRTY)
	return OK;

    add_dirty_coord(rdman, coord);

#if 0
    if(coord->flags & COF_HIDDEN)
	return OK;
#endif

    /* Make child coords dirty. */
    for(child = preorder_coord_subtree(coord, coord);
	child != NULL;
	child = preorder_coord_subtree(coord, child)) {
	if(child->flags & (COF_DIRTY | COF_HIDDEN)) {
	    preorder_coord_skip_subtree(child);
	    continue;
	}

	if(coord_is_cached(child)) {
	    preorder_coord_skip_subtree(child);
	    continue;
	}

	add_dirty_coord(rdman, child);
    }

    return OK;
}

static int _rdman_shape_changed(redraw_man_t *rdman, shape_t *shape) {
    geo_t *geo;
    int r;

    geo = shape->geo;

    if(geo->flags & GEF_DIRTY)
	return OK;

    r = add_dirty_geo(rdman, geo);
    if(r == ERR)
	return ERR;

    return OK;
}

/*! \brief Mark a shape is changed.
 *
 * The geo_t object of a changed shape is mark as dirty and
 * put into dirty_geos list.
 */
int rdman_shape_changed(redraw_man_t *rdman, shape_t *shape) {
    return _rdman_shape_changed(rdman, shape);
}

int rdman_paint_changed(redraw_man_t *rdman, paint_t *paint) {
    shnode_t *shnode;
    int r;

    FORPAINTMEMBERS(paint, shnode) {
	r = _rdman_shape_changed(rdman, shnode->shape);
	if(r != OK)
	    return ERR;
    }
    return OK;
}


/* Clean dirties */

static int is_coord_subtree_hidden(coord_t *coord) {
    while(coord) {
	if(coord->flags & COF_HIDDEN)
	    return 1;
	coord = coord->parent;
    }
    return 0;
}

static void clean_shape(shape_t *shape) {
    switch(MBO_TYPE(shape)) {
    case MBO_PATH:
	sh_path_transform(shape);
	break;
#ifdef SH_TEXT
    case MBO_TEXT:
	sh_text_transform(shape);
	break;
#endif
    case MBO_RECT:
	sh_rect_transform(shape);
	break;
    case MBO_IMAGE:
	sh_image_transform(shape);
	break;
#ifdef SH_STEXT
    case MBO_STEXT:
	sh_stext_transform(shape);
	break;
#endif
#ifdef UNITTEST
    default:
	sh_dummy_transform(shape);
	break;
#endif /* UNITTEST */
    }
    shape->geo->flags &= ~GEF_DIRTY;

    if(sh_get_flags(shape, GEF_HIDDEN) ||
       is_coord_subtree_hidden(shape->coord))
	sh_set_flags(shape, GEF_NOT_SHOWED);
    else
	sh_clear_flags(shape, GEF_NOT_SHOWED);
}

/*! \brief Setup canvas_info for the coord.
 *
 * Own a canvas or inherit it from parent.
 * \sa
 * - \ref redraw
 */
static void setup_canvas_info(redraw_man_t *rdman, coord_t *coord) {
    if(coord->parent == NULL)
	return;

    if(coord->opacity != 1 || coord_is_cached(coord)) {
	if(!coord_is_cached(coord)) {
	    /* canvas is assigned latter, in zeroing_coord() */
	    coord->canvas_info = coord_canvas_info_new(rdman, coord, NULL);
	    coord_set_flags(coord, COF_OWN_CANVAS);
	}
    } else {
	if(coord_is_cached(coord)) {
	    canvas_free(_coord_get_canvas(coord));
	    coord_canvas_info_free(rdman, coord->canvas_info);
	    coord_clear_flags(coord, COF_OWN_CANVAS);
	}
	/* This must here to keep coords that do not own canvas
	 * can always point to right canvas_info.  Since, they
	 * don't know when will parent change it's canvas_info.
	 */
	coord->canvas_info = coord->parent->canvas_info;
    }
}

/* \brief Compute matrix from cached canvas to parent device space.
 */
static void compute_cached_2_pdev_matrix(coord_t *coord,
					   co_aix canvas2pdev_matrix[6]) {
    coord_t *parent;
    co_aix *aggr;
    co_aix *matrix, *paggr;
    co_aix scale_x, scale_y;
    co_aix shift_x, shift_y;
    co_aix canvas2p[6];

    aggr = coord_get_aggr_matrix(coord);
    matrix = coord->matrix;
    parent = coord->parent;
    paggr = coord_get_aggr_matrix(parent);
    
    scale_x = matrix[0] / aggr[0];
    scale_y = matrix[3] / aggr[3];
    shift_x = matrix[2] - scale_x * aggr[2];
    shift_y = matrix[5] - scale_y * aggr[5];

    canvas2p[0] = scale_x;
    canvas2p[1] = 0;
    canvas2p[2] = shift_x;
    canvas2p[3] = 0;
    canvas2p[4] = scale_y;
    canvas2p[5] = shift_y;

    matrix_mul(paggr, canvas2p, canvas2pdev_matrix);
}

/*! \brief Compute area in parent cached coord for a cached coord.
 *
 * The coordination system of cached coord and descendants is resized,
 * and shifted.  It makes all descendants bound by a box, canvas box,
 * at 0, 0 and size is the same as the canvas.
 *
 * The bounding box where the canvas would be draw on the canvas on
 * ancestral cached coord can be retreived by shifting and resizing
 * canvas box in reverse and transform to coordination system of
 * ancestral cached coord.
 */ 
static void compute_pcache_area(coord_t *coord) {
    co_aix cached2pdev[6];
    int c_w, c_h;
    canvas_t *canvas;
    coord_canvas_info_t *canvas_info;
    co_aix poses[4][2];
    
    canvas_info = coord->canvas_info;
    SWAP(canvas_info->pcache_cur_area, canvas_info->pcache_last_area,
	 area_t *);
    compute_cached_2_pdev_matrix(coord, cached2pdev);
    
    canvas = _coord_get_canvas(coord);
    canvas_get_size(canvas, &c_w, &c_h);
    
    poses[0][0] = 0;
    poses[0][1] = 0;
    poses[1][0] = c_w;
    poses[1][1] = c_h;
    poses[2][0] = 0;
    poses[2][1] = c_h;
    poses[3][0] = c_w;
    poses[3][1] = 0;
    matrix_trans_pos(cached2pdev, &poses[0][0], &poses[0][1]);
    matrix_trans_pos(cached2pdev, &poses[1][0], &poses[1][1]);
    matrix_trans_pos(cached2pdev, &poses[2][0], &poses[2][1]);
    matrix_trans_pos(cached2pdev, &poses[3][0], &poses[3][1]);
    
    area_init(coord_get_pcache_area(coord), 4, poses);

    coord_set_flags(coord, COF_DIRTY_PCACHE_AREA);
}

/*! \brief Compute area of a coord.
 */
static int
compute_area(coord_t *coord) {
    static co_aix (*poses)[2];
    static int max_poses = 0;
    geo_t *geo;
    int cnt, pos_cnt;
    
    cnt = 0;
    FORMEMBERS(coord, geo) {
	cnt++;
    }
    
    if(max_poses < (cnt * 2)) {
	free(poses);
	max_poses = cnt * 2;
	poses = (co_aix (*)[2])malloc(sizeof(co_aix [2]) * max_poses);
	if(poses == NULL)
	    return ERR;
    }

    pos_cnt = 0;
    FORMEMBERS(coord, geo) {
	area_to_positions(geo->cur_area, poses + pos_cnt);
	pos_cnt += 2;
    }

    area_init(coord_get_area(coord), pos_cnt, poses);

    return OK;
}

static int coord_clean_members_n_compute_area(coord_t *coord) {
    geo_t *geo;
    int r;
    /*! \note poses is shared by invokings, it is not support reentrying. */
    
    /* Clean member shapes. */
    FORMEMBERS(coord, geo) {
	clean_shape(geo->shape);
    }

    r = compute_area(coord);
    if(r != OK)
	return ERR;

    return OK;
}

/*! \brief Clean dirty coords.
 *
 * This function compute aggregation matrix and area for dirty
 * coords. But, aggregation matrix of a cached coord is different from
 * normal one. (see compute_aggr_of_cached_coord()).
 *
 * \note coords their opacity != 1 are also traded as cached ones.
 */
static int clean_coord(redraw_man_t *rdman, coord_t *coord) {
    coord_t *child;
    int r;
    
    setup_canvas_info(rdman, coord);

    compute_aggr(coord);

    /* Areas of cached coords are computed in two phase.
     * Phase 1 works like other normal ones.  Phase 2, collects
     * all areas of descendants to compute a minimum covering area.
     * Phase 2 is performed by zeroing_coord().
     */
    r = coord_clean_members_n_compute_area(coord);
    if(r != OK)
	return ERR;

    add_dirty_area(rdman, coord, coord->cur_area);
    add_dirty_area(rdman, coord, coord->last_area);

    coord_clear_flags(coord, COF_DIRTY);
    coord_set_flags(coord, COF_JUST_CLEAN);
    
    FORCHILDREN(coord, child) {
	if(coord_is_cached(child))
	    add_dirty_pcache_area_coord(rdman, child);
    }
    
    return OK;
}

/*! \brief Clean coord_t objects.
 *
 * It computes aggregation matrix and area for dirty coords.
 *
 * This function also responsible for computing area of parent cached
 * coord, coord_canvas_info_t::pcache_cur_area, for its cached children.
 */
static int clean_rdman_coords(redraw_man_t *rdman) {
    coord_t *coord;
    coord_t **dirty_coords;
    int n_dirty_coords;
    int i, r;

    n_dirty_coords = rdman->dirty_coords.num;
    if(n_dirty_coords > 0) {
	dirty_coords = rdman->dirty_coords.ds;
	_insert_sort((void **)dirty_coords, n_dirty_coords,
		     OFFSET(coord_t, order)); /* ascend order */
	for(i = 0; i < n_dirty_coords; i++) {
	    coord = dirty_coords[i];
	    if(!coord_get_flags(coord, COF_DIRTY | COF_JUST_CLEAN))
		continue;
	    r = clean_coord(rdman, coord);
	    if(r != OK)
		return ERR;
	}
    }
    return OK;
}

static int clean_rdman_geos(redraw_man_t *rdman) {
    int i;
    int n_dirty_geos;
    geo_t **dirty_geos;
    geo_t *visit_geo;
    coord_t *coord;

    n_dirty_geos = rdman->dirty_geos.num;
    if(n_dirty_geos > 0) {
	dirty_geos = rdman->dirty_geos.ds;
	for(i = 0; i < n_dirty_geos; i++) {
	    visit_geo = dirty_geos[i];
	    if(!(visit_geo->flags & GEF_DIRTY))
		continue;

	    clean_shape(visit_geo->shape);
	    coord = geo_get_coord(visit_geo);
	    add_dirty_area(rdman, coord, visit_geo->cur_area);
	    add_dirty_area(rdman, coord, visit_geo->last_area);
	}
    }    

    return OK;
}

/*! \brief Shift space of coord to align left-top of minimum covering.
 *
 * Align left-top of minimum rectangle covering occupied area of
 * sub-graphic to origin of the space.
 */
static
void zeroing_coord(redraw_man_t *rdman, coord_t *coord) {
    coord_t *cur;
    area_t *area;
    geo_t *geo;
    co_aix min_x, min_y;
    co_aix max_x, max_y;
    co_aix x, y;
    int w, h;
    int c_w, c_h;
    mbe_t *canvas;
    co_aix *aggr;

    if(coord->parent == NULL)	/*! \note Should not zeroing root coord */
	abort();
    if(!(coord_is_zeroing(coord)))
	abort();

    coord_clear_zeroing(coord);

    /*
     * Compute minimum overing area of sub-graphic
     */
    area = coord_get_area(coord);
    min_x = area->x;
    min_y = area->y;
    max_x = min_x + area->w;
    max_y = min_y + area->h;

    for(cur = preorder_coord_subtree(coord, coord);
	cur != NULL;
	cur = preorder_coord_subtree(coord, cur)) {
	area = coord_get_area(cur);
	if(area->x < min_x)
	    min_x = area->x;
	if(area->y < min_y)
	    min_y = area->y;
	
	x = area->x + area->w;
	y = area->y + area->h;
	
	if(x > max_x)
	    max_x = x;
	if(y > max_y)
	    max_y = y;
	if(coord_is_cached(cur))
	    preorder_coord_skip_subtree(cur);
    }

    w = max_x - min_x;
    h = max_y - min_y;
    
    canvas = _coord_get_canvas(coord);
    if(canvas)
	canvas_get_size(canvas, &c_w, &c_h);
    else
	c_w = c_h = 0;

    /* Without COF_JUST_CLEAN means the coordination system and matrix
     * of the coord have not changed since last time of zeroing.  So,
     * if canvas box cover all descendants, we don't need rezeroing,
     * and avoid redraw all descendants.
     *
     * Width and height of actually drawing area should not be smaller
     * than half of canvas's width and height.
     */
    if(!coord_get_flags(coord, COF_JUST_CLEAN) &&
       min_x >= 0 && min_y >= 0 && max_x <= c_w && max_y <= c_h &&
       h >= (c_h >> 2) && w >= (c_w >> 2)) {
	/* Canvas fully cover sub-graphic. */
	coord_set_flags(coord, COF_SKIP_ZERO);
	return;
    }
    
    /*
     * Adjust matrics of descendants to align left-top corner of
     * minimum covering area with origin of space defined by
     * zeroing coord.
     */
    FOR_COORDS_PREORDER(coord, cur) {
	if(coord_is_cached(cur) && coord != cur) {
	    /*
	     * Cached coords are zeroed from root to leaves, so
	     * changes of aggr_matrix would be propagated to next
	     * level of cached.
	     */
	    preorder_coord_skip_subtree(cur);
	}
	/* Shift space */
	aggr = coord_get_aggr_matrix(cur);
	aggr[3] -= min_x;
	aggr[5] -= min_y;
	
	FOR_COORD_MEMBERS(coord, geo) {
	    /* \see GEO_SWAP() */
	    if(!geo_get_flags(geo, GEF_SWAP))
		SWAP(geo->cur_area, geo->last_area, area_t *);
	}
	coord_clean_members_n_compute_area(cur);
    }
    
    /*
     * Setup canvas
     *
     * Canvas of a cached coord is not setted in
     * coord_canvas_info_new().  It should be setted, here.
     */
    if(canvas == NULL || w > c_w || h > c_w) {
	if(canvas)
	    canvas_free(canvas);
	canvas = canvas_new(w, h);
	_coord_set_canvas(coord, canvas);
    }

    coord_set_flags(coord, COF_JUST_ZERO);
}

/*! \brief Add canvas owner of dirty geos to redraw_man_t::zeroing_coords.
 *
 * All possible coords that need a zeroing have at least one dirty geo.
 */
static int add_rdman_zeroing_coords(redraw_man_t *rdman) {
    int i;
    int n_dirty_geos;
    geo_t **dirty_geos, *geo;
    int n_dirty_coords;
    coord_t **dirty_coords, *coord;

    /* Mark all cached ancestral coords of dirty geos */
    n_dirty_geos = rdman->dirty_geos.num;
    dirty_geos = rdman->dirty_geos.ds;
    for(i = 0; i < n_dirty_geos; i++) {
	geo = dirty_geos[i];
	coord = coord_get_cached(geo_get_coord(geo));
	while(!coord_get_flags(coord, COF_MUST_ZEROING | COF_TEMP_MARK)) {
	    if(coord_is_root(coord))
		break;
	    coord_set_flags(coord, COF_TEMP_MARK);
	    coord = coord_get_cached(coord_get_parent(coord));
	}
    }
    
    /* Mark all cached ancestral coords of dirty coords */
    n_dirty_coords = rdman->dirty_coords.num;
    dirty_coords = rdman->dirty_coords.ds;
    for(i = 0; i < n_dirty_coords; i++) {
	coord = coord_get_cached(dirty_coords[i]);
	while(!coord_get_flags(coord, COF_MUST_ZEROING | COF_TEMP_MARK)) {
	    if(coord_is_root(coord))
		break;
	    coord_set_flags(coord, COF_TEMP_MARK);
	    coord = coord_get_cached(coord_get_parent(coord));
	}
    }
    
    /* Add all marked coords into redraw_man_t::zeroing_coords list */
    FOR_COORDS_PREORDER(rdman->root_coord, coord) {
	if(!coord_is_cached(coord) || coord_is_root(coord))
	    continue;		/* skip coords that is not cached */
	
	if(!coord_get_flags(coord, COF_TEMP_MARK)) {
	    if(coord_get_flags(coord, COF_DIRTY_PCACHE_AREA))
		add_dirty_pcache_area_coord(rdman, coord);
	    preorder_coord_skip_subtree(coord);
	    continue;
	}
	add_zeroing_coord(rdman, coord);
	
	coord_clear_flags(coord, COF_TEMP_MARK);
    }
    
    return OK;
}

/*! \brief Zeroing coords in redraw_man_t::zeroing_coords.
 *
 * \note redraw_man_t::zeroing_coords must in descent partial order of
 *	 tree.  The size of a cached coord is effected by cached
 *	 descendants.
 */
static int zeroing_rdman_coords(redraw_man_t *rdman) {
    int i;
    coords_t *all_zeroing;
    coord_t *coord;
   
    all_zeroing = &rdman->zeroing_coords;
    /*! Zeroing is performed from leaves to root.
     *
     * REASON: The size of canvas is also effected by cached
     *         descedants.  A cached coord is only effected by parent
     *         cached coord when it-self is dirty.  When a cached
     *         coord is dirty, it is clean (compute aggregated matrix)
     *         by recomputing a scale for x and y-axis from aggregated
     *         matrix of parent coord.  And, cleaning coord is
     *         performed before zeroing.  It means ancestors of a
     *         cached coord would not effect it when zeroing.
     */
    for(i = all_zeroing->num - 1; i >= 0; i--) {
	coord = all_zeroing->ds[i];
	zeroing_coord(rdman, coord);
	compute_pcache_area(coord);
    }
    
    return OK;
}

/*! \brief Compute pcache_area for coords whoes pcache_area is dirty.
 *
 * coord_t::dirty_pcache_area_coords also includes part of coords in
 * coord_t::zeroing_coords.  The pcache_area of coords that is in
 * coord_t::dirty_pcache_area_coords, but is not in
 * coord_t::zeroing_coords should be computed here.
 * zeroing_rdman_coords() is responsible for computing pcache_area for
 * zeroing ones.
 */
static int
compute_rdman_coords_pcache_area(redraw_man_t *rdman) {
    coords_t *all_coords;
    coord_t *coord;
    int i;
    
    all_coords = &rdman->dirty_pcache_area_coords;
    for(i = 0; i < all_coords->num; i++) {
	coord = all_coords->ds[i];
	if(coord_get_flags(coord, COF_DIRTY_PCACHE_AREA))
	    compute_pcache_area(coord);
    }
    return OK;
}

/*! \brief Add aggregated dirty areas to ancestor.
 *
 * Dirty areas are aggregated into two areas.  It assumes that even or odd
 * ones are old areas or new areas repsective.  So, all even ones are
 * aggregated in an area, and odd ones are in another.
 */
static void add_aggr_dirty_areas_to_ancestor(redraw_man_t *rdman,
					      coord_t *coord) {
    int i;
    int n_areas;
    co_aix poses0[2][2], poses1[2][2];
    co_aix canvas2pdev_matrix[6];
    area_t **areas, *area;
    area_t *area0, *area1;
    coord_t *parent, *pcached_coord;

    n_areas = _coord_get_dirty_areas(coord)->num;
    areas = _coord_get_dirty_areas(coord)->ds;
    if(n_areas == 0)
	abort();		/* should not happen! */
    
    area0 = _coord_get_aggr_dirty_areas(coord);
    area1 = area0 + 1;

    /* TODO: Since both cur & last area of coords are added into dirty
     *       area list, position of both areas shoud be adjusted for
     *       all descendants when zeroing a cached coord.
     */
    for(i = 0; i < n_areas; i++) {
	area = areas[i];
	if(area->w != 0 || area->h != 0)
	    break;
    }

    if(i >= n_areas)
	return;
    
    area = areas[i++];
    poses0[0][0] = area->x;
    poses0[0][1] = area->y;
    poses0[1][0] = area->x + area->w;
    poses0[1][1] = area->y + area->h;
    
    if(i < n_areas) {
	area = areas[i++];
	poses1[0][0] = area->x;
	poses1[0][1] = area->y;
	poses1[1][0] = area->x + area->w;
	poses1[1][1] = area->y + area->h;
    } else {
	poses1[0][0] = 0;
	poses1[0][1] = 0;
	poses1[1][0] = 0;
	poses1[1][1] = 0;
    }
    
    for(; i < n_areas - 1;) {
	/* Even areas */
	area = areas[i++];
	if(area->w != 0 || area->h != 0) {
	    poses0[0][0] = MB_MIN(poses0[0][0], area->x);
	    poses0[0][1] = MB_MIN(poses0[0][1], area->y);
	    poses0[1][0] = MB_MAX(poses0[1][0], area->x + area->w);
	    poses0[1][1] = MB_MAX(poses0[1][1], area->y + area->h);
	}
	/* Odd areas */
	area = areas[i++];
	if(area->w != 0 || area->h != 0) {
	    poses1[0][0] = MB_MIN(poses1[0][0], area->x);
	    poses1[0][1] = MB_MIN(poses1[0][1], area->y);
	    poses1[1][0] = MB_MAX(poses1[1][0], area->x + area->w);
	    poses1[1][1] = MB_MAX(poses1[1][1], area->y + area->h);
	}
    }
    
    if(i < n_areas) {
	area = areas[i];
	if(area->w != 0 || area->h != 0) {
	    poses0[0][0] = MB_MIN(poses0[0][0], area->x);
	    poses0[0][1] = MB_MIN(poses0[0][1], area->y);
	    poses0[1][0] = MB_MAX(poses0[1][0], area->x + area->w);
	    poses0[1][1] = MB_MAX(poses0[1][1], area->y + area->h);
	}
    }
    
    parent = coord_get_parent(coord);
    pcached_coord = coord_get_cached(parent);
    
    compute_cached_2_pdev_matrix(coord, canvas2pdev_matrix);

    /* Add dirty areas to parent cached coord. */
    matrix_trans_pos(canvas2pdev_matrix, poses0[0], poses0[0] + 1);
    matrix_trans_pos(canvas2pdev_matrix, poses0[1], poses0[1] + 1);
    area_init(area0, 2, poses0);
    add_dirty_area(rdman, pcached_coord, area0);
    
    matrix_trans_pos(canvas2pdev_matrix, poses1[0], poses1[0] + 1);
    matrix_trans_pos(canvas2pdev_matrix, poses1[1], poses1[1] + 1);
    area_init(area1, 2, poses1);
    add_dirty_area(rdman, pcached_coord, area1);

    if(coord_get_flags(coord, COF_JUST_CLEAN) &&
       !coord_get_flags(pcached_coord, COF_JUST_CLEAN))
	add_dirty_area(rdman, pcached_coord, coord->last_area);
}

/*! \brief To test if redrawing all elements on the canvas of a cached coord.
 */
#define IS_CACHE_REDRAW_ALL(co)					\
    (coord_get_flags((co), COF_JUST_CLEAN | COF_JUST_ZERO))

/* Aggregate dirty areas and propagate them to ancestor cached coord.
 *
 * The aggregation is performed from leaves to root.  But, this
 * function do not aggregate dirty areas for root coord.  The dirty
 * areas of a cached coord are aggregated into two areas, one for old
 * areas and one or new areas.  Both aggregation areas are add into
 * dirty_areas list of closet ancestral cached coord.
 */
static int add_rdman_aggr_dirty_areas(redraw_man_t *rdman) {
    int i;
    int n_zeroing;
    coord_t **zeroings;
    coord_t *coord, *pcached_coord;
    int n_dpca_coords;		/* number of dirty pcache area coords */
    coord_t **dpca_coords;	/* dirty pcache area coords */
    
    /* Add aggregated areas to parent cached one for coords in zeroing
     * list
     */
    n_zeroing = rdman->zeroing_coords.num;
    zeroings = rdman->zeroing_coords.ds;
    for(i = 0; i < n_zeroing; i++) {
	if(coord_get_flags(coord, COF_TEMP_MARK))
	    continue;
	coord_set_flags(coord, COF_TEMP_MARK);
	
	coord = zeroings[i];
	pcached_coord = coord_get_cached(coord_get_parent(coord));
	
	if(coord_is_root(coord) || IS_CACHE_REDRAW_ALL(pcached_coord))
	    continue;
	
	if(IS_CACHE_REDRAW_ALL(coord)) {
	    add_dirty_area(rdman, pcached_coord,
			   coord_get_pcache_area(coord));
	    add_dirty_area(rdman, pcached_coord,
			   coord_get_pcache_last_area(coord));
	} else {
	    add_aggr_dirty_areas_to_ancestor(rdman, coord);
	}
    }
    
    /* Add pcache_areas to parent cached one for coord that is
     * non-zeroing and its parent is changed.
     */
    n_dpca_coords = rdman->dirty_pcache_area_coords.num;
    dpca_coords = rdman->dirty_pcache_area_coords.ds;
    for(i = 0; i < n_dpca_coords; i++) {
	if(coord_get_flags(coord, COF_TEMP_MARK))
	    continue;
	coord_set_flags(coord, COF_TEMP_MARK);

	coord = dpca_coords[i];
	pcached_coord = coord_get_cached(coord_get_parent(coord));
	
	if(coord_is_root(coord) || IS_CACHE_REDRAW_ALL(pcached_coord))
	    continue;
	
	add_dirty_area(rdman, pcached_coord,
		       coord_get_pcache_area(coord));
	add_dirty_area(rdman, pcached_coord,
		       coord_get_pcache_last_area(coord));
    }

    /* Remove temporary mark */
    for(i = 0; i < n_zeroing; i++) {
	coord_clear_flags(zeroings[i], COF_TEMP_MARK);
    }
    for(i = 0; i < n_dpca_coords; i++) {
	coord_clear_flags(dpca_coords[i], COF_TEMP_MARK);
    }

    return OK;
}

/*! \brief Swap geo_t::cur_area and geo_t::last_area for a geo_t.
 *
 * It is call by rdman_clean_dirties() to swap areas for members of
 * dirty coord in redraw_man_t::dirty_coords and dirty geos in
 * redraw_man_t::dirty_geos.
 *
 * zeroing_coord() would also swap some areas for members of
 * descendants of a cached coord.  But, only members that was not
 * swapped, without GEF_SWAP flag, in this round of redrawing.
 * zeroing_coord() would not mark geos with GEF_SWAP since it not not
 * referenced later.  We don't mark geos in zeroing_coord() because we
 * don't want to unmark it later.  To unmark it, we should re-travel
 * forest of cached coords in redraw_man_t::zeroing_coords.  It is
 * expansive.
 */
#define GEO_SWAP(g)					\
    if(!geo_get_flags((g), GEF_SWAP)) {			\
	SWAP((g)->cur_area, (g)->last_area, area_t *);	\
	geo_set_flags((g), GEF_SWAP);			\
    }

/* \brief Clean dirty coords and shapes.
 *
 * The procedure of clean dirty coords and shapes include 3 major steps.
 *
 *   - Add dirty coords and shapes to rdman.
 *     - All descendants of a dirty coord are also dirty, except
 *       descendants of cached descendants.
 *   - Recompute aggregated transformation matrix from root to leaves
 *     for dirty coords.
 *     - The aggregated transformation matrix for a cached coord is
 *       different from other coords.
 *   - Compute new area for every dirty coord.
 *     - Area of a dirty coord is an aggregation of areas of all members.
 *     - A cached coord has two type of areas, one is for members of the cached
 *       coord, another one is for the block that cached coord and descendants
 *       will be mapped in parent cached coord.
 *       - Areas, for parent cached coord (pcache_cur_area), of
 *         non-dirty cached coord would be recomputed when cleaning
 *         parent coord.
 *       - Areas, for parent cached coord (pcache_cur_area), of dirty
 *         cached coord would be recomputed when zeroing the cached
 *         coord. (because zeroing would change aggregated matrix, and
 *         zeroing was performed after cleaning)
 *       - Areas, for members, of dirty cached coord would only be
 *         recomputed when cleaning the coord.
 *   - Perform zeroing on some cached coords that
 *     - dirty, is,
 *     - dirty descendants, has.
 *   - Propagate dirty areas to dirty area list of parent cached coord
 *     for every cached coords, not only for dirty cached coords.
 *
 * The cur_area of a cached coord is where members of the coord will
 * be draw in cache buffer, i.e. surface.  The area of the cached
 * coord and descendants is described by pcache_cur_area and
 * pcache_last_area in coord_canvas_info_t.
 */
static int rdman_clean_dirties(redraw_man_t *rdman) {
    int r;
    int i;
    coord_t **coords, *coord;
    geo_t **geos;
    geo_t *geo;

    /* coord_t::cur_area of coords are temporary pointed to
     * coord_canvas_info_t::owner_mems_area for store area
     * by clean_coord().
     */
    coords = rdman->dirty_coords.ds;
    for(i = 0; i < rdman->dirty_coords.num; i++) {
	coord = coords[i];
	SWAP(coord->cur_area, coord->last_area, area_t *);
	FOR_COORD_MEMBERS(coord, geo) {
	    GEO_SWAP(geo);
	}
    }
    
    geos = rdman->dirty_geos.ds;
    for(i = 0; i < rdman->dirty_geos.num; i++) {
	geo = geos[i];
	GEO_SWAP(geo);
    }
    
    r = clean_rdman_coords(rdman);
    if(r != OK)
	return ERR;

    /* TODO: save area of cached coord and descendants in
     *       cached_dirty_area for parent cached coord space.
     */

    r = clean_rdman_geos(rdman);
    if(r != OK)
	return ERR;

    /* Zeroing must be performed after clearing to get latest position
     * of shapes for computing new bounding box
     */
    r = add_rdman_zeroing_coords(rdman);
    if(r != OK)
	return ERR;

    r = zeroing_rdman_coords(rdman);
    if(r != OK)
	return ERR;

    r = compute_rdman_coords_pcache_area(rdman);
    if(r != OK)
	return ERR;
    
    r = add_rdman_aggr_dirty_areas(rdman);
    if(r != OK)
	return ERR;

    /*
     * Clear all flags setted by zeroing.
     */
    coords = rdman->dirty_coords.ds;
    for(i = 0; i < rdman->dirty_coords.num; i++) {
	coord = coords[i];
	coord_clear_flags(coord, COF_JUST_CLEAN);
	/* \see GEO_SWAP() */
	FOR_COORD_MEMBERS(coord, geo) {
	    geo_clear_flags(geo, GEF_SWAP);
	}
    }
    coords = rdman->zeroing_coords.ds;
    for(i = 0; i < rdman->zeroing_coords.num; i++)
	coord_clear_flags(coords[i],
			  COF_JUST_CLEAN | COF_JUST_ZERO | COF_SKIP_ZERO);
    coords = rdman->dirty_pcache_area_coords.ds;
    for(i = 0; i < rdman->dirty_pcache_area_coords.num; i++)
	coord_clear_flags(coords[i],
			  COF_JUST_CLEAN | COF_JUST_ZERO | COF_SKIP_ZERO);
    
    /* \see GEO_SWAP() */
    for(i = 0; i < rdman->dirty_geos.num; i++) {
	geo = geos[i];
	geo_clear_flags(geo, GEF_SWAP);
    }
    
    return OK;
}


/* Drawing and Redrawing
 * ============================================================
 */

#ifndef UNITTEST
static void set_shape_stroke_param(shape_t *shape, mbe_t *cr) {
    mbe_set_line_width(cr, shape->stroke_width);
}

static void fill_path_preserve(redraw_man_t *rdman) {
    mbe_fill_preserve(rdman->cr);
}

static void fill_path(redraw_man_t *rdman) {
    mbe_fill(rdman->cr);
}

static void stroke_path(redraw_man_t *rdman) {
    mbe_stroke(rdman->cr);
}
#else
static void set_shape_stroke_param(shape_t *shape, mbe_t *cr) {
}

static void fill_path_preserve(redraw_man_t *rdman) {
}

static void fill_path(redraw_man_t *rdman) {
}

static void stroke_path(redraw_man_t *rdman) {
}
#endif

static void draw_shape(redraw_man_t *rdman, mbe_t *cr, shape_t *shape) {
    paint_t *fill, *stroke;

    /*! \todo Move operator of shapes into singleton structures that define
     *	operators for them.
     */
    if(shape->fill || shape->stroke) {
	switch(MBO_TYPE(shape)) {
	case MBO_PATH:
	    sh_path_draw(shape, cr);
	    break;
#ifdef SH_TEXT
	case MBO_TEXT:
	    sh_text_draw(shape, cr);
	    break;
#endif
	case MBO_RECT:
	    sh_rect_draw(shape, cr);
	    break;
	case MBO_IMAGE:
	    sh_image_draw(shape, cr);
	    break;
#ifdef SH_STEXT
	case MBO_STEXT:
	    sh_stext_draw(shape, cr);
	    break;
#endif
#ifdef UNITTEST
	default:
	    sh_dummy_fill(shape, cr);
	    break;
#endif /* UNITTEST */
	}

	fill = shape->fill;
	if(shape->fill) {
	    fill->prepare(fill, cr);
	    if(shape->stroke)
		fill_path_preserve(rdman);
	    else
		fill_path(rdman);
	}

	stroke = shape->stroke;
	if(stroke) {
	    stroke->prepare(stroke, cr);
	    set_shape_stroke_param(shape, cr);
	    stroke_path(rdman);
	}
    } 
}

#ifndef UNITTEST
static void clear_canvas(canvas_t *canvas) {
    mbe_clear(canvas);
}

static void make_clip(mbe_t *cr, int n_dirty_areas,
		      area_t **dirty_areas) {
    int i;
    area_t *area;

    mbe_new_path(cr);
    for(i = 0; i < n_dirty_areas; i++) {
	area = dirty_areas[i];
	if(area->w < 0.1 || area->h < 0.1)
	    continue;
	mbe_rectangle(cr, area->x, area->y, area->w, area->h);
    }
    mbe_clip(cr);
}

static void reset_clip(canvas_t *cr) {
    mbe_reset_clip(cr);
}

static void copy_cr_2_backend(redraw_man_t *rdman, int n_dirty_areas,
			      area_t **dirty_areas) {
    if(n_dirty_areas)
	make_clip(rdman->backend, n_dirty_areas, dirty_areas);
    
    mbe_copy_source(rdman->cr, rdman->backend);
}
#else /* UNITTEST */
static void make_clip(mbe_t *cr, int n_dirty_areas,
		      area_t **dirty_areas) {
}

static void clear_canvas(canvas_t *canvas) {
}

static void reset_clip(canvas_t *cr) {
}

static void copy_cr_2_backend(redraw_man_t *rdman, int n_dirty_areas,
			      area_t **dirty_areas) {
}
#endif /* UNITTEST */

static void update_cached_canvas_2_parent(redraw_man_t *rdman,
					  coord_t *coord) {
    mbe_t *pcanvas, *canvas;
    mbe_surface_t *surface;
    mbe_pattern_t *pattern;
    co_aix reverse[6];
    co_aix canvas2pdev_matrix[6];

    if(coord_is_root(coord))
	return;

    compute_cached_2_pdev_matrix(coord, canvas2pdev_matrix);
    compute_reverse(canvas2pdev_matrix, reverse);
    
    canvas = _coord_get_canvas(coord);
    pcanvas = _coord_get_canvas(coord->parent);
    surface = mbe_get_target(canvas);
    pattern = mbe_pattern_create_for_surface(surface);
    mbe_pattern_set_matrix(pattern, reverse);
    mbe_set_source(pcanvas, pattern);
    mbe_paint_with_alpha(pcanvas, coord->opacity);
}

static int draw_coord_shapes_in_dirty_areas(redraw_man_t *rdman,
					    coord_t *coord) {
    int dirty = 0;
    int r;
    area_t **areas;
    int n_areas;
    mbe_t *canvas;
    geo_t *member;
    coord_t *child;
    int mem_idx;

    if(coord->flags & COF_HIDDEN)
	return OK;
    
    areas = _coord_get_dirty_areas(coord)->ds;
    n_areas = _coord_get_dirty_areas(coord)->num;
    canvas = _coord_get_canvas(coord);
    
    member = FIRST_MEMBER(coord);
    mem_idx = 0;
    child = FIRST_CHILD(coord);
    while(child != NULL || member != NULL) {
	if(child && child->before_pmem == mem_idx) {
	    if(coord_is_cached(child)) {
		if(!(child->flags & COF_HIDDEN) &&
		   is_area_in_areas(coord_get_area(child), n_areas, areas)) {
		    update_cached_canvas_2_parent(rdman, child);
		    dirty = 1;
		}
	    } else {
		r = draw_coord_shapes_in_dirty_areas(rdman, child);
		dirty |= r;
	    }
	    child = NEXT_CHILD(child);
	} else {
	    ASSERT(member != NULL);
	    if((!(member->flags & GEF_NOT_SHOWED)) &&
	       is_geo_in_areas(member, n_areas, areas)) {
		draw_shape(rdman, canvas, member->shape);
		dirty = 1;
	    }

	    member = NEXT_MEMBER(member);
	    mem_idx++;
	}
    }

    return dirty;
}

static int draw_dirty_cached_coord(redraw_man_t *rdman,
				   coord_t *coord) {
    area_t **areas, *area;
    area_t full_area;
    int n_areas;
    mbe_t *canvas;
    mbe_surface_t *surface;
    int i;
    int r;
    
    canvas = _coord_get_canvas(coord);
    
    if(IS_CACHE_REDRAW_ALL(coord)) {
	/*
	 * full_area covers all dirty areas of the cached coord.
	 */
	DARRAY_CLEAN(_coord_get_dirty_areas(coord));
	surface = mbe_get_target(canvas);
	full_area.x = 0;
	full_area.y = 0;
	full_area.w = mbe_image_surface_get_width(surface);
	full_area.h = mbe_image_surface_get_height(surface);
	add_dirty_area(rdman, coord, &full_area);
    }

    areas = _coord_get_dirty_areas(coord)->ds;
    n_areas = _coord_get_dirty_areas(coord)->num;
    
    for(i = 0; i < n_areas; i++) {
	area = areas[i];
	area->x = floorf(area->x);
	area->y = floorf(area->y);
	area->w = ceilf(area->w);
	area->h = ceilf(area->h);
    }

    make_clip(canvas, n_areas, areas);
    clear_canvas(canvas);

    r = draw_coord_shapes_in_dirty_areas(rdman, coord);
    
    reset_clip(canvas);

    return OK;
}

static void draw_shapes_in_dirty_areas(redraw_man_t *rdman) {
    int num;
    coord_t **zeroings;
    coord_t *coord;
    int i;

    zeroings = rdman->zeroing_coords.ds;
    num = rdman->zeroing_coords.num;
    /* Draw cached ones from leaves to root.
     * Since content of cached ones depend on descendants.
     */
    for(i = num - 1; i >= 0; i--) {
	coord = zeroings[i];
	if(coord_get_flags(coord, COF_TEMP_MARK))
	    continue;
	draw_dirty_cached_coord(rdman, coord);
	coord_set_flags(coord, COF_TEMP_MARK);
    }
    for(i = 0; i < num; i++)
	coord_clear_flags(coord, COF_TEMP_MARK);

    draw_dirty_cached_coord(rdman, rdman->root_coord);
}


/*! \brief Re-draw all changed shapes or shapes affected by changed coords.
 *
 * A coord object has a geo to keep track the range that it's members will
 * draw on.  Geo of a coord should be recomputed when the coord is changed.
 * Geo of a coord used to accelerate finding overlay shape objects of
 * a specified geo.  A coord object also must be recomputed when one of
 * it's members is changed.
 *
 * New and old geo values of a coord object that is recomputed for
 * changing of it-self must be used to find overlay shape objects.
 * New and old geo values of a shape should also be used to find
 * overlay shape objects, too.  If a shape's coord is changed, shape's
 * geo object is not used to find overlay shape objects any more.
 *
 * steps:
 * - update chagned coord objects
 * - recompute area for changed coord objects
 *   - recompute geo for members shape objects
 *   - clear dirty of geo for members to prevent from
 *     recomputing for change of shape objects.
 *   - add old and new area value to list of dirty areas.
 * - recompute geo for changed shape objects
 *   - only if a shape object is dirty.
 *   - put new and old value of area of geo to list of dirty areas.
 * - Scan all shapes and redraw shapes overlaid with dirty areas.
 *
 * dirty flag of coord objects is cleared after update.
 * dirty flag of geo objects is also cleared after recomputing.
 * Clean dirty flag can prevent redundant computing for geo and
 * corod objects.
 *
 */
int rdman_redraw_changed(redraw_man_t *rdman) {
    int r;
    event_t event;
    subject_t *redraw;
    int i;
    coord_t *coord;
    int n_areas;
    area_t **areas;
    
    r = rdman_clean_dirties(rdman);
    if(r != OK)
	return ERR;

    if(rdman->n_dirty_areas > 0) {
	/*! \brief Draw shapes in preorder of coord tree and support opacity
	 * rules.
	 */
	draw_shapes_in_dirty_areas(rdman);
	n_areas = _coord_get_dirty_areas(rdman->root_coord)->num;
	areas = _coord_get_dirty_areas(rdman->root_coord)->ds;
	copy_cr_2_backend(rdman, n_areas, areas);
	reset_clip(rdman->backend);
	for(i = 0; i < rdman->zeroing_coords.num; i++) {
	    coord = rdman->zeroing_coords.ds[i];
	    DARRAY_CLEAN(_coord_get_dirty_areas(coord));
	}
	DARRAY_CLEAN(_coord_get_dirty_areas(rdman->root_coord));
	rdman->n_dirty_areas = 0;
    }

    DARRAY_CLEAN(&rdman->dirty_coords);
    DARRAY_CLEAN(&rdman->dirty_geos);
    DARRAY_CLEAN(&rdman->zeroing_coords);
    DARRAY_CLEAN(&rdman->dirty_pcache_area_coords);
    
    /* Free postponsed removing */
    free_free_objs(rdman);

    redraw = rdman_get_redraw_subject(rdman);
    event.type = EVT_RDMAN_REDRAW;
    event.tgt = event.cur_tgt = redraw;
    subject_notify(redraw, &event);

    return OK;
}

/* NOTE: Before redrawing, the canvas/surface must be cleaned.
 * NOTE: After redrawing, the content must be copied to the backend surface.
 */

int rdman_redraw_all(redraw_man_t *rdman) {
    area_t area;
#ifndef UNITTEST
    mbe_surface_t *surface;
#endif
    int r;

    area.x = area.y = 0;
#ifndef UNITTEST
    surface = mbe_get_target(rdman->cr);
    area.w = mbe_image_surface_get_width(surface);
    area.h = mbe_image_surface_get_height(surface);
#else
    area.w = 1024;
    area.h = 1024;
#endif
    add_dirty_area(rdman, rdman->root_coord, &area);

    r = rdman_redraw_changed(rdman);
    if(r != OK)
	return ERR;

    return OK;
}

int rdman_redraw_area(redraw_man_t *rdman, co_aix x, co_aix y,
		      co_aix w, co_aix h) {
    area_t area;
    int r;

    area.x = x;
    area.y = y;
    area.w = w;
    area.h = h;
    add_dirty_area(rdman, rdman->root_coord, &area);

    r = rdman_redraw_changed(rdman);

    return r;
}

/*! \brief Helping function to travel descendant shapes of a coord.
 */
geo_t *rdman_geos(redraw_man_t *rdman, geo_t *last) {
    geo_t *next;
    coord_t *coord;
    
    if(last == NULL) {
	coord = rdman->root_coord;
	while(coord != NULL && FIRST_MEMBER(coord) == NULL)
	    coord = preorder_coord_subtree(rdman->root_coord, coord);
	if(coord == NULL)
	    return NULL;
	return FIRST_MEMBER(coord);
    }

    coord = last->shape->coord;
    next = NEXT_MEMBER(last);
    while(next == NULL) {
	coord = preorder_coord_subtree(rdman->root_coord, coord);
	if(coord == NULL)
	    return NULL;
	next = FIRST_MEMBER(coord);
    }
    return next;
}

int rdman_force_clean(redraw_man_t *rdman) {
    int r;

    r = rdman_clean_dirties(rdman);

    return r;
}

/*! \page man_obj Manage Objects.
 *
 * Shapes and paints should also be managed by redraw manager.  Redraw
 * manager must know life-cycle of shapes and paints to avoid to use them
 * after being free.  If a shape is released when it is dirty, redraw
 * manager will try to access them, after released, for redrawing.
 * We can make a copy information need by redraw manager to redraw them,
 * but it is more complicate, and induce runtime overhead.
 *
 * So, redraw manage had better also manage life-cycle of shapes and paints.
 * Shapes and paints should be created and freed through interfaces
 * provided by redraw manager.  To reduce overhead of interfaces, they can
 * be implemented as C macros.
 *
 * To refactory redraw manage to manage life-cycle of shapes and paints,
 * following functions/macros are introduced.
 * - rdman_paint_*_new()
 * - rdman_paint_free()
 * - rdman_shape_*_new()
 * - rdman_shape_free()
 */

/* \defgroup rdman_observer Observer memory management
 *
 * Implment factory and strategy functions for observers and subjects.
 * @{
 */
static subject_t *ob_subject_alloc(ob_factory_t *factory) {
    redraw_man_t *rdman;
    subject_t *subject;

    rdman = MEM2OBJ(factory, redraw_man_t, ob_factory);
    subject = elmpool_elm_alloc(rdman->subject_pool);

    return subject;
}

static void ob_subject_free(ob_factory_t *factory, subject_t *subject) {
    redraw_man_t *rdman;

    rdman = MEM2OBJ(factory, redraw_man_t, ob_factory);
    elmpool_elm_free(rdman->subject_pool, subject);
}

static observer_t *ob_observer_alloc(ob_factory_t *factory) {
    redraw_man_t *rdman;
    observer_t *observer;

    rdman = MEM2OBJ(factory, redraw_man_t, ob_factory);
    observer = elmpool_elm_alloc(rdman->observer_pool);

    return observer;
}

static void ob_observer_free(ob_factory_t *factory, observer_t *observer) {
    redraw_man_t *rdman;

    rdman = MEM2OBJ(factory, redraw_man_t, ob_factory);
    elmpool_elm_free(rdman->observer_pool, observer);
}

static subject_t *ob_get_parent_subject(ob_factory_t *factory,
					subject_t *cur_subject) {
    redraw_man_t *rdman;
    coord_t *coord, *parent_coord;
    geo_t *geo;
    subject_t *parent;

    rdman = MEM2OBJ(factory, redraw_man_t, ob_factory);
    switch(cur_subject->obj_type) {
    case OBJT_GEO:
	geo = (geo_t *)cur_subject->obj;
	parent_coord = geo->shape->coord;
	parent = parent_coord->mouse_event;
	break;
    case OBJT_COORD:
	coord = (coord_t *)cur_subject->obj;
	parent_coord = coord->parent;
	if(parent_coord == NULL) {
	    parent = NULL;
	    break;
	}
	parent = parent_coord->mouse_event;
	break;
    default:
	parent = NULL;
	break;
    }

    return parent;
}

/* @} */

/*! \brief Load an image as a paint_image_t.
 */
paint_t *rdman_img_ldr_load_paint(redraw_man_t *rdman, const char *img_id) {
    mb_img_data_t *img_data;
    paint_t *paint;
    mb_img_ldr_t *ldr = rdman_img_ldr(rdman);
    
    img_data = MB_IMG_LDR_LOAD(ldr, img_id);
    if(img_data == NULL)
	return NULL;
    
    paint = rdman_paint_image_new(rdman, img_data);
    if(paint == NULL)
	MB_IMG_DATA_FREE(img_data);
    
    return paint;
}

#ifdef UNITTEST
/* Test cases */

#include <CUnit/Basic.h>

struct _sh_dummy {
    shape_t shape;
    co_aix x, y;
    co_aix w, h;
    int trans_cnt;
    int draw_cnt;
};

void sh_dummy_free(shape_t *sh) {
    free(sh);
}

shape_t *sh_dummy_new(redraw_man_t *rdman,
		      co_aix x, co_aix y, co_aix w, co_aix h) {
    sh_dummy_t *dummy;

    dummy = (sh_dummy_t *)malloc(sizeof(sh_dummy_t));
    if(dummy == NULL)
	return NULL;

    memset(dummy, 0, sizeof(sh_dummy_t));

    dummy->x = x;
    dummy->y = y;
    dummy->w = w;
    dummy->h = h;
    dummy->trans_cnt = 0;
    dummy->draw_cnt = 0;
    dummy->shape.free = sh_dummy_free;
    
    rdman_shape_man(rdman, (shape_t *)dummy);

    return (shape_t *)dummy;
}

void sh_dummy_transform(shape_t *shape) {
    sh_dummy_t *dummy = (sh_dummy_t *)shape;
    co_aix poses[2][2];
    co_aix x1, y1, x2, y2;
    
    if(shape->geo && shape->coord) {
	x1 = dummy->x;
	y1 = dummy->y;
	x2 = x1 + dummy->w;
	y2 = y1 + dummy->h;

	coord_trans_pos(shape->coord, &x1, &y1);
	coord_trans_pos(shape->coord, &x2, &y2);
	poses[0][0] = x1;
	poses[0][1] = y1;
	poses[1][0] = x2;
	poses[1][1] = y2;
    
	if(shape->geo)
	    geo_from_positions(shape->geo, 2, poses);
    }
    dummy->trans_cnt++;
}

void sh_dummy_fill(shape_t *shape, mbe_t *cr) {
    sh_dummy_t *dummy;

    dummy = (sh_dummy_t *)shape;
    dummy->draw_cnt++;
}

static void dummy_paint_prepare(paint_t *paint, mbe_t *cr) {
}

static void dummy_paint_free(redraw_man_t *rdman, paint_t *paint) {
    if(paint)
	free(paint);
}

paint_t *dummy_paint_new(redraw_man_t *rdman) {
    paint_t *paint;

    paint = (paint_t *)malloc(sizeof(paint_t));
    if(paint == NULL)
	return NULL;

    paint_init(paint, MBP_DUMMY, dummy_paint_prepare, dummy_paint_free);

    return paint;
}

static void test_rdman_redraw_changed(void) {
    coord_t *coords[3];
    shape_t *shapes[3];
    sh_dummy_t **dummys;
    paint_t *paint;
    redraw_man_t *rdman;
    redraw_man_t _rdman;
    int i;

    dummys = (sh_dummy_t **)shapes;

    rdman = &_rdman;
    redraw_man_init(rdman, NULL, NULL);
    paint = dummy_paint_new(rdman);
    for(i = 0; i < 3; i++) {
	shapes[i] = sh_dummy_new(rdman, 0, 0, 50, 50);
	rdman_paint_fill(rdman, paint, shapes[i]);
	coords[i] = rdman_coord_new(rdman, rdman->root_coord);
	coords[i]->matrix[2] = 10 + i * 100;
	coords[i]->matrix[5] = 10 + i * 100;
	rdman_coord_changed(rdman, coords[i]);
	rdman_add_shape(rdman, shapes[i], coords[i]);
    }
    rdman_redraw_all(rdman);
    CU_ASSERT(dummys[0]->trans_cnt == 1);
    CU_ASSERT(dummys[1]->trans_cnt == 1);
    CU_ASSERT(dummys[2]->trans_cnt == 1);
    CU_ASSERT(dummys[0]->draw_cnt == 1);
    CU_ASSERT(dummys[1]->draw_cnt == 1);
    CU_ASSERT(dummys[2]->draw_cnt == 1);
    
    coords[2]->matrix[2] = 100;
    coords[2]->matrix[5] = 100;
    rdman_coord_changed(rdman, coords[0]);
    rdman_coord_changed(rdman, coords[2]);
    rdman_redraw_changed(rdman);

    CU_ASSERT(dummys[0]->draw_cnt == 2);
    CU_ASSERT(dummys[1]->draw_cnt == 2);
    CU_ASSERT(dummys[2]->draw_cnt == 2);

    rdman_paint_free(rdman, paint);
    redraw_man_destroy(rdman);
}

static int test_free_pass = 0;

static void test_free(redraw_man_t *rdman, void *obj) {
    test_free_pass++;
}

static void test_rdman_free_objs(void) {
    redraw_man_t *rdman;
    redraw_man_t _rdman;
    int i;

    redraw_man_init(&_rdman, NULL, NULL);
    rdman = &_rdman;

    test_free_pass = 0;

    for(i = 0; i < 4; i++)
	add_free_obj(rdman, NULL, test_free);

    redraw_man_destroy(rdman);
    CU_ASSERT(test_free_pass == 4);
}

CU_pSuite get_redraw_man_suite(void) {
    CU_pSuite suite;

    suite = CU_add_suite("Suite_redraw_man", NULL, NULL);
    CU_ADD_TEST(suite, test_rdman_redraw_changed);
    CU_ADD_TEST(suite, test_rdman_free_objs);

    return suite;
}

#endif /* UNITTEST */
