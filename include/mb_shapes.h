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
#include "mb_img_ldr.h"
#include <pango/pangocairo.h>

/*! \page define_shape How to Define Shapes
 *
 * A shape implementation must include
 * - rdman_shape_*_new()
 *   - clear memory for shape_t member.
 *     - mb_obj_init() to initialize shape_t::obj.
 *   - assign *_free() to \ref shape_t::free.
 *   - make new object been managed by a redraw manager.
 *     - call rdman_shape_man()
 * - *_free()
 *   - assigned to \ref shape_t::free.
 * - *_transform()
 * - *_draw()
 *   - *_draw() is responsive to define shape.  How the shape is filled
 *     or stroked is defined by paint.
 * - first member variable of a shape type must be a shape_t.
 * 
 * Must modify
 * - event.c::draw_shape_path()
 * - redraw_man.c::clean_shape()
 * - redraw_man.c::draw_shape()
 *
 * \section shape_transform Shape Transform
 *
 * All shape types must have a shape transform function.  It is invoked by
 * redraw_man.c::clean_shape().  It's task is to update \ref geo_t of the
 * shape object.  In most situtation, it call geo_from_positions() to
 * update geo_t.
 * 
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
/*! \brief Change the content of the text element.
 *  In the SVG, the content of a text tag can be composed of several tspan inside it. The Madbutterfly parser will collect all content of a 
 *  text segement as a single string. The attribute of these characters are saved in a seperate data structure. In the program level, we will
 *  not keep the SVG text tree. Instead, all attributes will be expanded into a list. 
 *
 *  When you change the content of a text element, please remember that the attributes will not be removed by the way. You need to change 
 *  them seperately. 
 *
 */
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


#define TEXTALIGN_START       1
#define TEXTALIGN_MIDDLE      2
#define TEXTALIGN_END         3

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

/*! \brief Change the style of the text.
 *
 *  This function can add a couple of attributes to a segment of text or the whole text field. If the @end is -1, the attributes 
 *  will be applied to the whole text field. The @style should be initialized by using the mb_textstyle_xxx functions. All attributes
 *  which is not initialized will not be changed. It means that the @style will be added into all existing style instead of reaplcing
 *  it.
 */
extern void sh_text_set_style(shape_t *shape,int begin,int end,mb_textstyle_t *format);
/*! \brief Change the color of the text field
 *  Change the color of the whole text field. This will removed all existing color attribute. If you want to change part of the text 
 *  field only, please use the sh_text_set_style instead.
 */
extern void sh_text_set_color(shape_t *shape, unsigned color);
/*! \brief Turn on/off the bold attribute.
 *  Turn on/off the font weight of the whole text field. This will removed all existing bold setting. If you want to change part of the text 
 *  field only, please use the sh_text_set_style instead.
 */
extern void sh_text_set_bold(shape_t *shape, int bold);
/*! \brief Turn on/off the italic attribute.
 *  Turn on/off the italic of the whole text field. This will removed all existing italic setting. If you want to change part of the text 
 *  field only, please use the sh_text_set_style instead.
 */
extern void sh_text_set_italic(shape_t *shape, int italic);
/*! \brief Turn on/off the underline attribute.
 *  Turn on/off the underline of the whole text field. This will removed all existing underline setting. If you want to change part of the text 
 *  field only, please use the sh_text_set_style instead.
 */
extern void sh_text_set_underline(shape_t *shape, int underline);
/*! \brief Change the font of the text field.
 *  Change the font of the whole text field. This will removed all existing underline setting. If you want to change part of the text 
 *  field only, please use the sh_text_set_style instead.
 */
extern void sh_text_set_font(shape_t *shape, char *family);
/*! \brief Init the text style data structure.
 *
 *  This is usually used to initialize the mb_textstyle_t which is allocate in the stack. It will mark all property as undefined. All undefined
 *  property will not change when the sh_text_set_style is called.
 *
 */
static inline void mb_textstyle_init(mb_textstyle_t *style)
{
    style->property = 0;
}
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
#define TEXTCOLOR_RED(c) (((c)&0xff0000)>>16)
#define TEXTCOLOR_GREEN(c) (((c)&0xff00)>>8)
#define TEXTCOLOR_BLUE(c) (((c)&0xff))
#define TEXTCOLOR_RGB(r,g,b) (((r)<<16)|((g)<<8)|(b))
static inline void mb_textstyle_set_color(mb_textstyle_t *style, unsigned int color)
{
    style->property |= TEXTSTYLE_COLOR;
    style->color = color;
}
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

/*! \defgroup shape_image Shape of Image
 * @{
 */
extern shape_t *rdman_shape_image_new(redraw_man_t *rdman,
				      co_aix x, co_aix y,
				      co_aix w, co_aix h);
extern void sh_image_transform(shape_t *shape);
extern void sh_image_draw(shape_t *shape, cairo_t *cr);
extern void sh_image_set_geometry(shape_t *shape, co_aix x, co_aix y,
				  co_aix w, co_aix h);
/* @} */

/*! \defgroup shape_stext A Simple Implementation of Shape of Image
 * @{
 */

/*! \defgroup font_face Define font face used to describe style of text.
 * @{
 */
/*! \brief Font face of MadButterfly.
 *
 * It actully a cairo_font_face_t, now.  But, it can be change for latter.
 * So, programmer should not depend on cairo_font_face_t.
 */
typedef struct _mb_font_face mb_font_face_t;

enum MB_FONT_SLANTS {
    MB_FONT_SLANT_DUMMY,
    MB_FONT_SLANT_ROMAN,
    MB_FONT_SLANT_ITALIC,
    MB_FONT_SLANT_OBLIQUE,
    MB_FONT_SLANT_MAX
};

extern mb_font_face_t *mb_font_face_query(redraw_man_t *rdman,
					  const char *family,
					  int slant,
					  int weight);
extern void mb_font_face_free(mb_font_face_t *face);
/* @} */

/*! \brief Describe style of a block of text.
 *
 * \ref sh_stext_t describes style of a text by a list of
 * \ref mb_style_blk_t.
 */
typedef struct {
    int n_chars;
    mb_font_face_t *face;
    co_aix font_sz;
} mb_style_blk_t;

extern shape_t *rdman_shape_stext_new(redraw_man_t *rdman,
				      co_aix x, co_aix y,
				      const char *txt);
extern void sh_stext_transform(shape_t *shape);
extern void sh_stext_draw(shape_t *shape, cairo_t *cr);
extern int sh_stext_set_text(shape_t *shape, const char *txt);
extern int sh_stext_set_style(shape_t *shape,
			       const mb_style_blk_t *blks,
			       int nblks);

/* @} */
/* @} */

#endif /* __SHAPES_H_ */
