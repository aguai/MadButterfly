#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <cairo.h>
#include <pango/pangocairo.h>
#include "mb_types.h"
#include "mb_shapes.h"

#define ASSERT(x)
#define OK 0
#define ERR -1

typedef struct _sh_text {
    shape_t shape;
    char *data;
    co_aix x, y;
    co_aix d_x, d_y;		/* device x and y */
    co_aix font_size;
    co_aix d_font_size;
    cairo_font_face_t *face;
    cairo_scaled_font_t *scaled_font;
    int flags;
    PangoLayout *layout;
    int align;
    PangoAttrList *attrs;
} sh_text_t;

#define TXF_SCALE_DIRTY 0x1

static void sh_text_free(shape_t *shape) {
    sh_text_t *text = (sh_text_t *)shape;

    if(text->scaled_font)
	cairo_scaled_font_destroy(text->scaled_font);
    cairo_font_face_destroy(text->face);
}

static void sh_text_P_generate_layout(sh_text_t *text,cairo_t *cr);
shape_t *rdman_shape_text_new(redraw_man_t *rdman,
			      const char *txt, co_aix x, co_aix y,
			      co_aix font_size, cairo_font_face_t *face,PangoAttrList *attrs) {
    sh_text_t *text;

    text = (sh_text_t *)malloc(sizeof(sh_text_t));
    if(text == NULL)
	return NULL;


    memset(text, 0, sizeof(sh_text_t));
    mb_obj_init(text, MBO_TEXT);
    text->data = strdup(txt);
    if(text->data == NULL) {
	free(text);
	return NULL;
    }
    text->x = x;
    text->y = y;
    text->font_size = font_size;
    cairo_font_face_reference(face);
    text->face = face;
    text->flags |= TXF_SCALE_DIRTY;

    text->shape.free = sh_text_free;
    text->layout = NULL;
    text->attrs = attrs;
    text->align = TEXTALIGN_START;
    
    rdman_shape_man(rdman, (shape_t *)text);

    return (shape_t *)text;
}

void sh_text_set_pos(shape_t *shape, co_aix x, co_aix y)
{
    sh_text_t *text = (sh_text_t *)shape;
    text->x = x;
    text->y = y;
}


void sh_text_get_pos(shape_t *shape, co_aix *x, co_aix *y)
{
    sh_text_t *text = (sh_text_t *)shape;
    *x = text->x;
    *y = text->y;
}

void sh_text_get_text(shape_t *shape, char *text,int size)
{
    sh_text_t *t = (sh_text_t *)shape;
    strncpy(text,t->data, size);
}

void sh_text_set_text(shape_t *shape, const char *txt) {
    sh_text_t *text = (sh_text_t *)shape;
    char *buf;

    buf = strdup(txt);
    if(text->data) free(text->data);
    text->data = buf;
}

void sh_text_set_color(shape_t *shape, unsigned int color)
{
    PangoAttribute *attr = pango_attr_foreground_new(TEXTCOLOR_RED(color)<<8,TEXTCOLOR_GREEN(color)<<8,TEXTCOLOR_BLUE(color)<<8);
    sh_text_t *text = (sh_text_t *)shape;
    attr->start_index = 0;
    attr->end_index = -1;
    pango_attr_list_change(text->attrs, attr);
}
void sh_text_set_bold(shape_t *shape,int bold)
{
    PangoAttribute *attr = pango_attr_weight_new(bold? PANGO_WEIGHT_BOLD:PANGO_WEIGHT_NORMAL);
    sh_text_t *text = (sh_text_t *)shape;
    attr->start_index = 0;
    attr->end_index = -1;
    pango_attr_list_change(text->attrs, attr);
}
void sh_text_set_italic(shape_t *shape,int italic)
{
    PangoAttribute *attr = pango_attr_style_new(italic? PANGO_STYLE_ITALIC:PANGO_STYLE_NORMAL);
    sh_text_t *text = (sh_text_t *)shape;
    attr->start_index = 0;
    attr->end_index = -1;
    pango_attr_list_change(text->attrs, attr);
}
void sh_text_set_underline(shape_t *shape,int underline)
{
    PangoAttribute *attr = pango_attr_underline_new(underline? PANGO_UNDERLINE_SINGLE:PANGO_UNDERLINE_NONE);
    sh_text_t *text = (sh_text_t *)shape;
    attr->start_index = 0;
    attr->end_index = -1;
    pango_attr_list_change(text->attrs, attr);
}
void sh_text_set_font(shape_t *shape,char *family)
{
    PangoAttribute *attr = pango_attr_family_new(family);
    sh_text_t *text = (sh_text_t *)shape;
    attr->start_index = 0;
    attr->end_index = -1;
    pango_attr_list_change(text->attrs, attr);
}

void sh_text_set_style(shape_t *shape,int begin,int end,mb_textstyle_t *format)
{
    PangoAttribute *attr;
    sh_text_t *text = (sh_text_t *)shape;

    if (end == -1) {
	end = strlen(text->data);
    } else
	end++;
    if (format->property & TEXTSTYLE_BOLD) {
	attr = pango_attr_weight_new(PANGO_WEIGHT_BOLD);
	attr->start_index = begin;
	attr->end_index = end;
	pango_attr_list_change(text->attrs,attr);
    }
    if (format->property & TEXTSTYLE_ITALIC) {
	attr = pango_attr_style_new(PANGO_STYLE_ITALIC);
	attr->start_index = begin;
	attr->end_index = end;
	pango_attr_list_change(text->attrs,attr);
    }
    if (format->property & TEXTSTYLE_UNDERLINE) {
	attr = pango_attr_underline_new(PANGO_UNDERLINE_SINGLE);
	attr->start_index = begin;
	attr->end_index = end;
	pango_attr_list_change(text->attrs,attr);
    }
    if (format->property & TEXTSTYLE_COLOR) {
	printf("color=%x\n", format->color);
	printf("red = %x\n",TEXTCOLOR_RED(format->color));
	printf("green = %x\n",TEXTCOLOR_GREEN(format->color));
	printf("blue = %x\n",TEXTCOLOR_BLUE(format->color));
	attr = pango_attr_foreground_new(TEXTCOLOR_RED(format->color)<<8,TEXTCOLOR_GREEN(format->color)<<8,TEXTCOLOR_BLUE(format->color)<<8);
	attr->start_index = begin;
	attr->end_index = end;
	pango_attr_list_change(text->attrs,attr);
    }
    if (format->property & TEXTSTYLE_FONT) {
	attr = pango_attr_family_new(format->font);
	attr->start_index = begin;
	attr->end_index = end;
	pango_attr_list_change(text->attrs,attr);
    }
    if (format->property & TEXTSTYLE_ALIGN) {
	// We can have one align style for the whole text only
	if (begin != 0 || (end != strlen(text->data)-1))
	    return;
	text->align = format->align;
    }
}

static int get_extents(sh_text_t *text, PangoRectangle *extents) {
    cairo_matrix_t fmatrix;
    cairo_matrix_t ctm;
    cairo_scaled_font_t *new_scaled;
    cairo_font_options_t *fopt;

    pango_layout_get_extents(text->layout, NULL, extents);
    pango_extents_to_pixels(extents,NULL);
    return OK;
}

void sh_text_transform(shape_t *shape) {
    sh_text_t *text;
    coord_t *coord;
    canvas_t *canvas;
    co_aix x, y;
    co_aix shw;
    PangoRectangle extents;
    co_aix poses[2][2];
    int r;

    text = (sh_text_t *)shape;
    
    text->d_font_size = coord_trans_size(shape->coord, text->font_size);
    
    coord = sh_get_coord(shape);
    canvas = _coord_get_canvas(coord);
    sh_text_P_generate_layout(text, (cairo_t *)canvas);

    x = text->x;
    y = text->y;
    coord_trans_pos(shape->coord, &x, &y);
    r = get_extents(text, &extents);

    //printf("x=%f y=%f text=%s ascent=%d,descent=%d,width=%d height=%d\n", x,y,text->data,PANGO_ASCENT(extents), PANGO_DESCENT(extents), extents.width, extents.height);
    ASSERT(r == OK);

    text->d_x = x-5;
    text->d_y = y-PANGO_DESCENT(extents)+5;
    shw = shape->stroke_width / 2;
    /* FIXME: It is unreasonable that a font exceed it's bbox.
     * We add 5 pixels in get right bbox.  But, it is unreasonable.
     */

    poses[0][0] = text->d_x + extents.x - shw;
    poses[0][1] = text->d_y + extents.y - shw;
    poses[1][0] = poses[0][0] + extents.width +  shape->stroke_width;
    poses[1][1] = poses[0][1] + extents.height +  shape->stroke_width;
    geo_from_positions(shape->geo, 2, poses);
    /*! \todo Support ratation for shape_text. */
}

static void sh_text_P_generate_layout(sh_text_t *text,cairo_t *cr)
{
    PangoAttribute *attr;
    PangoAttrList *attrlist;
    PangoFontDescription *desc;

    if (text->layout) {
        g_object_unref(text->layout);
    }
    text->layout = pango_cairo_create_layout(cr);
    desc = pango_font_description_from_string("Sans Bold");
    //cairo_set_source_rgb (cr, 0, 0, 0);
    pango_layout_set_font_description (text->layout, desc);
    pango_layout_set_text(text->layout,text->data,strlen(text->data));
    pango_layout_set_attributes(text->layout, text->attrs);
    pango_cairo_update_layout(cr,text->layout);
    printf("text=%s\n",text->data);
}
static void draw_text(sh_text_t *text, cairo_t *cr) {
    cairo_move_to(cr, text->d_x, text->d_y);
    pango_cairo_layout_path(cr,text->layout);
}


void sh_text_draw(shape_t *shape, cairo_t *cr) {
    draw_text((sh_text_t *)shape, cr);
}

