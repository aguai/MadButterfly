/*! \file
 * \brief Declare interfaces of shapes.
 *
 * \todo Add ellipse shape.
 * \todo Add circle shape.
 */
#ifndef __SHAPES_H_
#define __SHAPES_H_

#include <cairo.h>
#include "mb_types.h"
#include "mb_redraw_man.h"
#include <pango/pangocairo.h>

/*! \page define_shape How to Define Shapes
 *
 * A shape implementation must include
 * - rdman_shape_*_new()
 *   - clear memory for shape_t member.
 *   - assign *_free() to \ref shape_t::free.
 *   - make new object been managed by a redraw manager.
 *     - call rdman_shape_man()
 * - *_free()
 *   - assigned to \ref shape_t::free.
 * - *_transform()
 * - *_draw()
 * - first member variable of a shape type must be a shape_t.
 * 
 * Must modify
 * - event.c::draw_shape_path()
 * - redraw_man.c::clean_shape()
 * - redraw_man.c::draw_shape()
 */

/*! \defgroup shapes Shapes
 * @{
 */

/*! \defgroup shape_path Shape of Path
 * @{
 */
extern shape_t *rdman_shape_path_new(redraw_man_t *rdman, char *data);
extern shape_t *rdman_shape_path_new_from_binary(redraw_man_t *rdman, char *commands, co_aix *arg,int  arg_cnt,int *fix_arg,int fix_arg_cnt);
extern void sh_path_transform(shape_t *shape);
extern void sh_path_draw(shape_t *shape, cairo_t *cr);
/* @} */

/*! \defgroup shape_text Shape of Text
 * @{
 */
extern shape_t *rdman_shape_text_new(redraw_man_t *rdman,
				     const char *txt, co_aix x, co_aix y,
				     co_aix font_size,
				     cairo_font_face_t *face,PangoAttrList *attrs);
extern void sh_text_set_text(shape_t *shape, const char *txt);
extern void sh_text_transform(shape_t *shape);
extern void sh_text_draw(shape_t *shape, cairo_t *cr);
/* @} */

/*! \defgroup mb_text_t Shape of Text
 * @{
 */
#define TEXTSTYLE_BOLD        1
#define TEXTSTYLE_ITALIC      2
#define TEXTSTYLE_UNDERLINE   4
#define TEXTSTYLE_COLOR       8
#define TEXTSTYLE_FONT        0x10
#define TEXTSTYLE_ALIGN       0x20

typedef struct {
    int property;
    unsigned int color;
    unsigned int align;
    char *font;
} mb_textstyle_t;

typedef struct _textsegment {
    int x;
    int y;
    mb_textstyle_t style;
    int size;
    char *buf;
    struct _textsegment *next;
} mb_text_segment_t;

#define MBTEXT_DIRTY 1

typedef struct {
    int nseg;
    mb_text_segment_t *segs;
    int flag;
    cairo_surface_t *surface;
} mb_text_t;

extern void mb_textstyle_init(mb_textstyle_t *style);
extern void mb_textstyle_set_font(mb_textstyle_t *style, char *font);
static inline char *mb_textstyle_get_font(mb_textstyle_t *style)
{
    if (style->property & TEXTSTYLE_FONT)
        return style->font;
    else
        return NULL;
}
extern void mb_textstyle_set_bold(mb_textstyle_t *style, int bold);
static inline int mb_textstyle_get_bold(mb_textstyle_t *style) 
{
    return style->property & TEXTSTYLE_BOLD;
}
extern void mb_textstyle_set_italic(mb_textstyle_t *style, int italic);
static inline int mb_textstyle_get_italic(mb_textstyle_t *style)
{
    return style->property & TEXTSTYLE_ITALIC;
}
extern void mb_textstyle_set_underline(mb_textstyle_t *style, int underline);
static inline int mb_textstyle_get_undeline(mb_textstyle_t *style)
{
    return style->property & TEXTSTYLE_UNDERLINE;
}
extern void mb_textstyle_set_color(mb_textstyle_t *style, unsigned int color);
static inline unsigned int mb_textstyle_get_color(mb_textstyle_t *style)
{
    if (style->property & TEXTSTYLE_COLOR)
        return style->color;
    else
        return 0;
}

static inline int mb_textstyle_has_color(mb_textstyle_t *style)
{
    return style->property & TEXTSTYLE_COLOR;
}
extern void mb_textstyle_set_alignment(mb_textstyle_t *style, int alignment);
extern int mb_textstyle_get_alignment(mb_textstyle_t *style);



extern void mb_text_set_style(mb_text_t *text, int begin,int end,mb_textstyle_t *style);
extern void mb_text_get_style(mb_text_t *text, int n,mb_textstyle_t *style);
extern void mb_text_set_text(mb_text_t *text, char *string,int begin,int end);
extern void mb_text_get_text(mb_text_t *text, int begin,int end, char *string);


/* @} */

/*! \defgroup shape_rect Shape of Rectangle
 * @{
 */
extern shape_t *rdman_shape_rect_new(redraw_man_t *rdman,
				     co_aix x, co_aix y,
				     co_aix w, co_aix h,
				     co_aix rx, co_aix ry);
extern void sh_rect_transform(shape_t *shape);
extern void sh_rect_draw(shape_t *shape, cairo_t *cr);
extern void sh_rect_set(shape_t *shape, co_aix x, co_aix y,
			co_aix w, co_aix h, co_aix rx, co_aix ry);
/* @} */
/* @} */

#endif /* __SHAPES_H_ */
