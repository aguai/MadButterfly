#include <stdio.h>
#include "mb_graph_engine.h"
#include <cairo-ft.h>
#include "mb_shapes.h"
#include "mb_tools.h"

#ifdef UNITTEST

#define NO_DOX(x) x
#define UT_FAKE(r, x, param)			\
    r x param {}

UT_FAKE(void, sh_stext_transform, (shape_t *shape));
UT_FAKE(void, sh_stext_draw, (shape_t *shape, mbe_t *cr));

typedef struct _ut_area {
    co_aix x, y;
    co_aix w, h;
} ut_area_t;
#define area_t ut_area_t

typedef struct _ut_shape {
    co_aix aggr[6];
    void (*free)(struct _ut_shape *);
    ut_area_t area;
} ut_shape_t;
#define shape_t ut_shape_t

#undef sh_get_aggr_matrix
#define sh_get_aggr_matrix(sh) (sh)->aggr

#undef sh_get_area
#define sh_get_area(sh) (&(sh)->area)

#undef mb_obj_init
#define mb_obj_init(o, t)
#undef rdman_shape_man
#define rdman_shape_man(rdman, sh)

#define rdman_shape_stext_new ut_rdman_shape_stext_new
#define sh_stext_transform ut_sh_stext_transform
#define sh_stext_draw ut_sh_stext_draw
#define sh_stext_set_text ut_sh_stext_set_text
#define sh_stext_set_style ut_sh_stext_set_style

#undef mbe_get_scaled_font
#define mbe_get_scaled_font(cr) ((mbe_scaled_font_t *)NULL)
#undef mbe_set_scaled_font
#define mbe_set_scaled_font(cr, scaled) ((mbe_scaled_font_t *)NULL)
#undef mbe_scaled_font_reference
#define mbe_scaled_font_reference(x)
#define MAX_MOVE 32
NO_DOX(static co_aix move_xys[MAX_MOVE][2]);
NO_DOX(static int move_cnt = 0);
#undef mbe_move_to
#define mbe_move_to(cr, x, y)			\
    do {					\
	move_xys[move_cnt][0] = x;		\
	move_xys[move_cnt++][1] = y;		\
    } while(0)
#undef mbe_scaled_font_destroy
#define mbe_scaled_font_destroy(scaled)
#undef mbe_text_path
#define mbe_text_path(cr, buf)

#endif /* UNITTEST */

#ifndef ASSERT
#define ASSERT(x)
#endif
#define OK 0
#define ERR -1

/*! \page stext Simple Text
 *
 * A sh_stext_t is broken into fragments.  Each fragment comprises the text
 * and the styles applied on the fragement.  The styles determines font face
 * used to show the text.  The fragment of text with styles is called
 * styled block.
 */

/*! \defgroup fontconfig_freetype Fontconfig and FreeType Layer.
 *
 * This layer implements a font provider to rest of the system.
 * It bases on fontconfig and FreeType supporting of Cairo.
 * If you want to provide stext with technologies other than fontconfig and
 * FreeType, just replace this layer with the implmenetation that you want.
 *
 * @{
 */
/*! \brief Stakeholder of scaled font.
 *
 * It is actually a mbe_scaled_font_t, now.  But, it should not be
 * noticed by out-siders.  Only \ref fontconfig_freetype
 * should known it.
 */
typedef struct _mb_scaled_font mb_scaled_font_t;

/*! \brief Stakeholder of scaled font.
 *
 * Although, mb_text_extents_t is defined as a mbe_scaled_font_t, but
 * programmers should assume it is opague.
 * 
 * An extents is the span of showing a fragement of text on the output device.
 * It includes x and y advance of cursor after showinng the text.
 * The cursor maybe not really existed.  But, the advance is computed as
 * the cursor existed.  It also includes width and height of the text.
 * The bearing of a styled block is the left-top corner of the bounding box.
 * The bounding box of a styled block is the minimal rectangle, on the
 * output device, that can contain the text.  The bearing is related to
 * the base line for an extents.
 */
typedef mbe_text_extents_t mb_text_extents_t;

#define MBE_GET_X_ADV(ext) ((ext)->x_advance)
#define MBE_GET_Y_ADV(ext) ((ext)->y_advance)
#define MBE_GET_X_BEARING(ext) ((ext)->x_bearing)
#define MBE_GET_Y_BEARING(ext) ((ext)->y_bearing)
#define MBE_GET_WIDTH(ext) ((ext)->width)
#define MBE_GET_HEIGHT(ext) ((ext)->height)
#define MBE_SET_X_ADV(ext, v) do { ((ext)->x_advance) = v; } while(0)
#define MBE_SET_Y_ADV(ext, v) do { ((ext)->y_advance) = v; } while(0)
#define MBE_SET_X_BEARING(ext, v) do { ((ext)->x_bearing) = v; } while(0)
#define MBE_SET_Y_BEARING(ext, v) do { ((ext)->y_bearing) = v; } while(0)
#define MBE_SET_WIDTH(ext, v) do { ((ext)->width) = v; } while(0)
#define MBE_SET_HEIGHT(ext, v) do { ((ext)->height) = v; } while(0)

static mb_font_face_t *
query_font_face(const char *family, int slant, int weight) {
    return (mb_font_face_t *)mbe_query_font_face(family, slant, weight);
}

static void
free_font_face(mb_font_face_t *face) {
    ASSERT(face == NULL);

    mbe_free_font_face((mbe_font_face_t *)face);
}

/*! \brief This is scaled font for specified size and extent.
 *
 * Font face only specified which font would be used to draw text
 * message.  But, it also need to scale glyphs to specified size and
 * rotation.  This function return a scaled font specified by a
 * matrix that transform glyph from design space of the font to
 * user space of cairo surface.
 */
static
mb_scaled_font_t *make_scaled_font_face_matrix(mb_font_face_t *face,
					       co_aix *matrix) {
    mbe_scaled_font_t *scaled_font;
    mbe_matrix_t font_matrix;
    static mbe_matrix_t id = {
	1, 0,
	0, 1,
	0, 0
    };
    
    ASSERT(matrix != NULL);
    
    font_matrix.xx = *matrix++;
    font_matrix.xy = *matrix++;
    font_matrix.x0 = *matrix++;
    font_matrix.yx = *matrix++;
    font_matrix.yy = *matrix++;
    font_matrix.y0 = *matrix;
    scaled_font = mbe_scaled_font_create((mbe_font_face_t *)face,
					 &font_matrix, &id);

    return (mb_scaled_font_t *)scaled_font;
}

static
void scaled_font_free(mb_scaled_font_t *scaled_font) {
    mbe_scaled_font_destroy((mbe_scaled_font_t *)scaled_font);
}

static
void compute_text_extents(mb_scaled_font_t *scaled_font, const char *txt,
			  mb_text_extents_t *extents) {
    mbe_scaled_font_text_extents((mbe_scaled_font_t *)scaled_font,
				   txt,
				   (mbe_text_extents_t *)extents);
}

static
mb_text_extents_t *mb_text_extents_new(void) {
    mbe_text_extents_t *extents;

    extents = (mbe_text_extents_t *)malloc(sizeof(mbe_text_extents_t));
    return extents;
}

static
void mb_text_extents_free(mb_text_extents_t *extents) {
    free(extents);
}

static
void draw_text_scaled(mbe_t *cr, const char *txt, int tlen,
		      mb_scaled_font_t *scaled, co_aix x, co_aix y) {
    mbe_scaled_font_t *saved_scaled;
    int total_tlen;
    const char *buf;

    total_tlen = strlen(txt);
    if(total_tlen > tlen)
	buf = strndup(txt, tlen);
    else
	buf = txt;
    
    saved_scaled = mbe_get_scaled_font(cr);
    mbe_scaled_font_reference(saved_scaled);
    mbe_set_scaled_font(cr, (mbe_scaled_font_t *)scaled);
    
    mbe_move_to(cr, x, y);
    mbe_text_path(cr, buf);
    
    mbe_set_scaled_font(cr, saved_scaled);
    mbe_scaled_font_destroy(saved_scaled);

    if(total_tlen > tlen)
	free((char *)buf);
}

/* @} */

/*! \brief Query and return a font face for a specified attribute vector.
 *
 * Programmers use mb_font_face_t to specify fonts used to show a
 * block of text on the output device.  They can get mb_font_face_t with
 * this function.  The objects return by mb_font_face_query() should be
 * freed with mb_font_face_free() when they are not more used.
 *
 * \param family is the name of a font family (times).
 * \param slant is one of \ref MB_FONT_SLANTS.
 * \param weight decides if a font is thin or heavy.
 */
mb_font_face_t *mb_font_face_query(redraw_man_t *rdman,
				   const char *family,
				   int slant,
				   int weight) {
    return query_font_face(family, slant, weight);
}

void mb_font_face_free(mb_font_face_t *face) {
    ASSERT(face != NULL);
    free_font_face(face);
}

DARRAY(scaled_fonts_lst, mb_scaled_font_t *);
DARRAY_DEFINE(scaled_fonts_lst, mb_scaled_font_t *);
DARRAY(style_blks_lst, mb_style_blk_t);
DARRAY_DEFINE_ADV(style_blks_lst, mb_style_blk_t);
DARRAY(extents_lst, mb_text_extents_t);
DARRAY_DEFINE_ADV(extents_lst, mb_text_extents_t);

/*! \brief A simple implementation of text shape.
 *
 */
typedef struct _sh_stext {
    shape_t shape;
    const char *txt;		      /*!< \brief Text to be showed */
    style_blks_lst_t style_blks;
    co_aix x, y;
    co_aix dx, dy;
    scaled_fonts_lst_t scaled_fonts;
    mb_text_extents_t extents;
    extents_lst_t sub_exts;
} sh_stext_t;

static
void _rdman_shape_stext_free(shape_t *shape) {
    sh_stext_t *txt_o = (sh_stext_t *)shape;
    int i;

    DARRAY_DESTROY(&txt_o->style_blks);
    
    for(i = 0; i < txt_o->scaled_fonts.num; i++)
	scaled_font_free(txt_o->scaled_fonts.ds[i]);
    DARRAY_DESTROY(&txt_o->scaled_fonts);
    DARRAY_DESTROY(&txt_o->sub_exts);
    
    if(txt_o->txt)
	free((void *)txt_o->txt);

    free(txt_o);
}

shape_t *rdman_shape_stext_new(redraw_man_t *rdman, const char *txt,
			       co_aix x, co_aix y) {
    sh_stext_t *txt_o;

    ASSERT(txt != NULL);
    
    txt_o = (sh_stext_t *)malloc(sizeof(sh_stext_t));
    if(txt_o == NULL)
	return NULL;

    memset(&txt_o->shape, 0, sizeof(shape_t));
    mb_obj_init(txt_o, MBO_STEXT);
    
    txt_o->txt = strdup(txt);
    DARRAY_INIT(&txt_o->style_blks);
    txt_o->x = x;
    txt_o->y = y;
    DARRAY_INIT(&txt_o->scaled_fonts);
    DARRAY_INIT(&txt_o->sub_exts);

    if(txt_o->txt == NULL) {
	free(txt_o);
	txt_o = NULL;
    }

    txt_o->shape.free = _rdman_shape_stext_free;
    
    rdman_shape_man(rdman, (shape_t *)txt_o);
    
    return (shape_t *)txt_o;
}

static
int compute_utf8_chars_sz(const char *txt, int n_chars) {
    int i;
    const char *p = txt;
    const char *v;
    
    for(i = 0; i < n_chars && *p; i++) {
	if(!(*p & 0x80))	/* single byte */
	    p++;
	else if((*p & 0xe0) == 0xc0) /* 2 bytes */
	    p += 2;
	else if((*p & 0xf0) == 0xe0) /* 3 bytes */
	    p += 3;
	else if((*p & 0xf8) == 0xf0) /* 4 bytes */
	    p += 4;
	else
	    return ERR;
    }
    if(i < n_chars)
	return ERR;

    for(v = txt; v != p; v++)
	if(*v == '\x0')
	    return ERR;
    
    return p - txt;
}

static
mb_scaled_font_t *make_scaled_font_face(sh_stext_t *txt_o,
					   mb_font_face_t *face,
					   co_aix shift_x, co_aix shift_y,
					   co_aix font_sz) {
    co_aix matrix[6], scaled_matrix[6];
    co_aix *aggr;
    co_aix noshift_aggr[6];
    mb_scaled_font_t *scaled;

    aggr = sh_get_aggr_matrix((shape_t *)txt_o);
    memcpy(noshift_aggr, aggr, sizeof(co_aix) * 6);
    noshift_aggr[2] = 0;
    noshift_aggr[5] = 0;
    
    matrix[0] = font_sz;
    matrix[1] = 0;
    matrix[2] = shift_x;
    matrix[3] = 0;
    matrix[4] = font_sz;
    matrix[5] += shift_y;
    matrix_mul(noshift_aggr, matrix, scaled_matrix);
    
    scaled = make_scaled_font_face_matrix(face, scaled_matrix);

    return scaled;
}

/*! \brief Extend an extents from another sub-extents.
 *
 * A styled text is styled by several styled blocks, so extents of
 * blocks should be computed separated, collected, and aggreagated
 * into a full extents.
 */
static
void extent_extents(mb_text_extents_t *full, mb_text_extents_t *sub) {
    co_aix f_rbx, f_rby;	/* rb stands for right bottom */
    co_aix s_rbx, s_rby;
    co_aix s_xbr, s_ybr;
    co_aix new_x_adv, new_y_adv;

    f_rbx = MBE_GET_X_BEARING(full) + MBE_GET_WIDTH(full);
    f_rby = MBE_GET_Y_BEARING(full) + MBE_GET_HEIGHT(full);
    s_xbr = MBE_GET_X_BEARING(sub) + MBE_GET_X_ADV(full);
    s_ybr = MBE_GET_Y_BEARING(sub) + MBE_GET_Y_ADV(full);
    s_rbx = s_xbr + MBE_GET_WIDTH(sub);
    s_rby = s_ybr + MBE_GET_HEIGHT(sub);
    
    /* set bearing */
    if(MBE_GET_X_BEARING(full) > s_xbr)
	MBE_SET_X_BEARING(full, s_xbr);
    if(MBE_GET_Y_BEARING(full) > s_ybr)
	MBE_SET_Y_BEARING(full, s_ybr);
    
    /* set width/height */
    if(f_rbx < s_rbx)
	MBE_SET_WIDTH(full, s_rbx - MBE_GET_X_BEARING(full));
    else
	MBE_SET_WIDTH(full, f_rbx - MBE_GET_X_BEARING(full));
    if(f_rby < s_rby)
	MBE_SET_HEIGHT(full, s_rby - MBE_GET_Y_BEARING(full));
    else
	MBE_SET_HEIGHT(full, f_rby - MBE_GET_Y_BEARING(full));

    /* set x/y advance */
    new_x_adv = MBE_GET_X_ADV(full) + MBE_GET_X_ADV(sub);
    new_y_adv = MBE_GET_Y_ADV(full) + MBE_GET_Y_ADV(sub);
    MBE_SET_X_ADV(full, new_x_adv);
    MBE_SET_Y_ADV(full, new_y_adv);
}

/*! \brief Compute extents of a stext object according style blocks.
 *
 * It create scaled fonts for style blocks, compute their extents,
 * and compute where they should be draw acoording advance of style
 * blocks before a style block.
 *
 * The scaled font that is created by this function for style blocks
 * are drawed at origin.  So, extents of these blocks are also based
 * on that the text are drawed at origin of user space.  But,
 * aggreagated extents (sh_stext_t::extents) accounts x/y advances
 * to create correct extents for full text string with style blocks.
 *
 * This function assumes texts are always drawed at 0, 0. So,
 * transform function should adjust x and y bearing corresponding
 * x, y of text to compute area.
 */
static
void compute_styled_extents_n_scaled_font(sh_stext_t *txt_o) {
    mb_style_blk_t *blk;
    style_blks_lst_t *style_blks;
    int blk_txt_len;
    mb_scaled_font_t *scaled;
    scaled_fonts_lst_t *scaled_fonts;
    extents_lst_t *sub_exts;
    mb_text_extents_t *sub;
    char *txt, saved;
    int i, nscaled;
    
    scaled_fonts = &txt_o->scaled_fonts;
    for(i = 0; i < scaled_fonts->num; i++)
	scaled_font_free(scaled_fonts->ds[i]);
    DARRAY_CLEAN(scaled_fonts);
    
    style_blks = &txt_o->style_blks;
    blk = style_blks->ds;

    sub_exts = &txt_o->sub_exts;
    DARRAY_CLEAN(sub_exts);
    extents_lst_adv(sub_exts, style_blks->num);
    
    txt = (char *)txt_o->txt;
    for(i = 0; i < style_blks->num; i++) {
	scaled = make_scaled_font_face(txt_o, blk->face,
				       0, 0, blk->font_sz);
	ASSERT(scaled != NULL);
	scaled_fonts_lst_add(scaled_fonts, scaled);
	sub = sub_exts->ds + i;
	
	blk_txt_len = compute_utf8_chars_sz(txt, blk->n_chars);
	ASSERT(blk_txt_len != ERR);
	
	saved = txt[blk_txt_len];
	txt[blk_txt_len] = 0;
	compute_text_extents(scaled, txt, sub);
	txt[blk_txt_len] = saved;

	blk++;
	txt += blk_txt_len;
    }
    
    if(style_blks->num > 0) {
	sub = sub_exts->ds;
	memcpy(&txt_o->extents, sub, sizeof(mb_text_extents_t));
	for(i = 1; i < style_blks->num; i++) {
	    sub = sub_exts->ds + i;
	    extent_extents(&txt_o->extents, sub);
	}
    } else
	memset(&txt_o->extents, sizeof(mb_text_extents_t), 0);    
}

/*
 * What we have to do in sh_stext_transform() is
 * - computing bounding box for the text,
 * - computing offset x,y for the text of style blocks,
 * - free old scaled fonts, and
 * - making scaled fonts for style blocks.
 *
 * Extents of style blocks are computed with x,y at 0,0.
 * So, we should add x,y, after transforming by the aggreagated matrix,
 * to output area.
 */
void sh_stext_transform(shape_t *shape) {
    sh_stext_t *txt_o = (sh_stext_t *)shape;
    area_t *area;
    mb_text_extents_t *ext;
    co_aix *aggr;

    ASSERT(txt_o != NULL);

    aggr = sh_get_aggr_matrix(shape);
    
    txt_o->dx = txt_o->x;
    txt_o->dy = txt_o->y;
    matrix_trans_pos(aggr, &txt_o->dx, &txt_o->dy);
    
    compute_styled_extents_n_scaled_font(txt_o);
    ext = &txt_o->extents;
    
    area = sh_get_area(shape);
    area->x = MBE_GET_X_BEARING(ext) + txt_o->dx;
    area->y = MBE_GET_Y_BEARING(ext) + txt_o->dy;
    area->w = MBE_GET_WIDTH(ext);
    area->h = MBE_GET_HEIGHT(ext);
}

void sh_stext_draw(shape_t *shape, mbe_t *cr) {
    sh_stext_t *txt_o = (sh_stext_t *)shape;
    co_aix x, y;
    const char *txt;
    scaled_fonts_lst_t *scaled_fonts;
    mb_scaled_font_t *scaled;
    style_blks_lst_t *style_blks;
    mb_style_blk_t *blk;
    mb_text_extents_t *ext;
    int blk_txt_len;
    int i;

    ASSERT(txt_o != NULL);
    
    x = txt_o->dx;
    y = txt_o->dy;
    txt = txt_o->txt;
    scaled_fonts = &txt_o->scaled_fonts;
    style_blks = &txt_o->style_blks;
    ext = txt_o->sub_exts.ds;
    
    for(i = 0; i < scaled_fonts->num; i++) {
	scaled = scaled_fonts->ds[i];
	blk = style_blks->ds + i;
	blk_txt_len = compute_utf8_chars_sz(txt, blk->n_chars);
	draw_text_scaled(cr, txt, blk_txt_len, scaled, x, y);
	
	x += MBE_GET_X_ADV(ext);
	y += MBE_GET_Y_ADV(ext);
	txt += blk_txt_len;
    }
}

int sh_stext_set_text(shape_t *shape, const char *txt) {
    sh_stext_t *txt_o = (sh_stext_t *)shape;
    char *new_txt;
    int sz;

    ASSERT(txt_o != NULL);
    ASSERT(txt != NULL);
    
    sz = strlen(txt) + 1;
    new_txt = (char *)realloc((void *)txt_o->txt, sz);
    if(new_txt == NULL)
	return ERR;
    
    memcpy(new_txt, txt, sz);
    txt_o->txt = new_txt;
    
    return OK;
}

int sh_stext_set_style(shape_t *shape,
			const mb_style_blk_t *blks,
			int nblks) {
    sh_stext_t *txt_o = (sh_stext_t *)shape;
    style_blks_lst_t *style_blks;
    mb_style_blk_t *new_blks;
    int sz;

    ASSERT(txt_o != NULL);
    ASSERT(nblks >= 0);
    
    style_blks = &txt_o->style_blks;
    DARRAY_CLEAN(style_blks);
    style_blks_lst_adv(style_blks, nblks);
    
    memcpy(style_blks->ds,
	   blks, nblks * sizeof(mb_style_blk_t));
	   
    return OK;
}

#ifdef UNITTEST

#include <CUnit/Basic.h>

static
void test_query_font_face(void) {
    mb_font_face_t *face;
    mbe_status_t status;

    face = query_font_face("serif", MB_FONT_SLANT_ROMAN, 100);
    CU_ASSERT(face != NULL);
    status = mbe_font_face_status((mbe_font_face_t *)face);
    CU_ASSERT(status == MBE_STATUS_SUCCESS);
    
    free_font_face(face);
}

static
void test_make_scaled_font_face_matrix(void) {
    co_aix matrix[6] = {5, 0, 0, 0, 5, 0};
    mb_font_face_t *face;
    mb_scaled_font_t *scaled;
    mbe_status_t status;

    face = query_font_face("serif", MB_FONT_SLANT_ROMAN, 100);
    CU_ASSERT(face != NULL);
    status = mbe_font_face_status((mbe_font_face_t *)face);
    CU_ASSERT(status == MBE_STATUS_SUCCESS);
    
    scaled = make_scaled_font_face_matrix(face, matrix);
    CU_ASSERT(scaled != NULL);
    status = mbe_scaled_font_status((mbe_scaled_font_t *)scaled);
    CU_ASSERT(status == MBE_STATUS_SUCCESS);
    
    scaled_font_free(scaled);
    free_font_face(face);
}

static
void test_compute_text_extents(void) {
    co_aix matrix[6] = {10, 0, 0, 0, 10, 0};
    co_aix x_adv1, x_adv2;
    co_aix x_bearing1, x_bearing2;
    mb_font_face_t *face;
    mb_scaled_font_t *scaled;
    mb_text_extents_t ext;

    face = query_font_face("serif", MB_FONT_SLANT_ROMAN, 100);
    CU_ASSERT(face != NULL)
    scaled = make_scaled_font_face_matrix(face, matrix);
    CU_ASSERT(scaled != NULL);

    compute_text_extents(scaled, "test", &ext);
    CU_ASSERT(MBE_GET_HEIGHT(&ext) >= 5 && MBE_GET_HEIGHT(&ext) <= 12);
    CU_ASSERT(MBE_GET_WIDTH(&ext) >= 16 && MBE_GET_WIDTH(&ext) <= 48);
    x_adv1 = MBE_GET_X_ADV(&ext);
    x_bearing1 = MBE_GET_X_BEARING(&ext);
    scaled_font_free(scaled);

    matrix[2] = 5;
    scaled = make_scaled_font_face_matrix(face, matrix);
    CU_ASSERT(scaled != NULL);

    compute_text_extents(scaled, "test", &ext);
    CU_ASSERT(MBE_GET_HEIGHT(&ext) >= 5 && MBE_GET_HEIGHT(&ext) <= 12);
    CU_ASSERT(MBE_GET_WIDTH(&ext) >= 16 && MBE_GET_WIDTH(&ext) <= 48);
    x_adv2 = MBE_GET_X_ADV(&ext);
    x_bearing2 = MBE_GET_X_BEARING(&ext);
    scaled_font_free(scaled);

    CU_ASSERT(x_adv1 == x_adv2);
    CU_ASSERT((x_bearing1 + 5) == x_bearing2);

    free_font_face(face);
}

static
void test_extent_extents(void) {
    mb_text_extents_t ext1, ext2;

    MBE_SET_WIDTH(&ext1, 20);
    MBE_SET_HEIGHT(&ext1, 10);
    MBE_SET_X_BEARING(&ext1, 1);
    MBE_SET_Y_BEARING(&ext1, -8);
    MBE_SET_X_ADV(&ext1, 21);
    MBE_SET_Y_ADV(&ext1, -3);
    
    MBE_SET_WIDTH(&ext2, 30);
    MBE_SET_HEIGHT(&ext2, 11);
    MBE_SET_X_BEARING(&ext2, 2);
    MBE_SET_Y_BEARING(&ext2, -11);
    MBE_SET_X_ADV(&ext2, 32);
    MBE_SET_Y_ADV(&ext2, -5);
    
    extent_extents(&ext1, &ext2);

    CU_ASSERT(MBE_GET_WIDTH(&ext1) == 52);
    CU_ASSERT(MBE_GET_HEIGHT(&ext1) == 16);
    CU_ASSERT(MBE_GET_X_BEARING(&ext1) == 1);
    CU_ASSERT(MBE_GET_Y_BEARING(&ext1) == -14);
    CU_ASSERT(MBE_GET_X_ADV(&ext1) == 53);
    CU_ASSERT(MBE_GET_Y_ADV(&ext1) == -8);
}

static
void test_compute_utf8_chars_sz(void) {
    const char *str = "\xe4\xb8\xad\xe6\x96\x87test\xe6\xb8\xac\xe8\xa9\xa6";
    int sz;
    
    sz = compute_utf8_chars_sz(str, 4);
    CU_ASSERT(sz == 8);

    sz = compute_utf8_chars_sz(str, 9);
    CU_ASSERT(sz == ERR);
}

static
void test_compute_styled_extents_n_scaled_font(void) {
    sh_stext_t *txt_o;
    co_aix *aggr;
    mb_style_blk_t blks[2];
    mb_font_face_t *face;
    mb_text_extents_t *ext;
    int r;

    txt_o = (sh_stext_t *)rdman_shape_stext_new((redraw_man_t *)NULL,
						"Hello World", 10, 15);
    CU_ASSERT(txt_o != NULL);

    aggr = txt_o->shape.aggr;
    aggr[0] = 1;
    aggr[1] = 0;
    aggr[2] = 0;
    aggr[3] = 0;
    aggr[4] = 1;
    aggr[5] = 0;

    face = query_font_face("serif", MB_FONT_SLANT_ROMAN, 100);
    CU_ASSERT(face != NULL);
    
    blks[0].n_chars = 5;
    blks[0].face = face;
    blks[0].font_sz = 10;
    
    blks[1].n_chars = 4;
    blks[1].face = face;
    blks[1].font_sz = 15.5;
    
    r = sh_stext_set_style((shape_t *)txt_o, blks, 2);
    CU_ASSERT(r == OK);

    compute_styled_extents_n_scaled_font(txt_o);

    ext = &txt_o->extents;
    CU_ASSERT(MBE_GET_HEIGHT(ext) > 11);
    CU_ASSERT(MBE_GET_HEIGHT(ext) < 20);
    CU_ASSERT(MBE_GET_WIDTH(ext) > 36);
    CU_ASSERT(MBE_GET_WIDTH(ext) < 72);
    CU_ASSERT(MBE_GET_X_ADV(ext) > 36);
    CU_ASSERT(MBE_GET_X_ADV(ext) < 72);
    CU_ASSERT(MBE_GET_Y_ADV(ext) == 0);
    
    _rdman_shape_stext_free((shape_t *)txt_o);
    free_font_face(face);
}

static
void test_compute_styled_extents_n_scaled_font_rotate(void) {
    sh_stext_t *txt_o;
    co_aix *aggr;
    mb_style_blk_t blks[2];
    mb_font_face_t *face;
    mb_text_extents_t *ext;
    int r;

    txt_o = (sh_stext_t *)rdman_shape_stext_new((redraw_man_t *)NULL,
						"Hello World", 10, 15);
    CU_ASSERT(txt_o != NULL);

    aggr = txt_o->shape.aggr;
    aggr[0] = 1;
    aggr[1] = 0;
    aggr[2] = 0;
    aggr[3] = 1;
    aggr[4] = 1;
    aggr[5] = 0;

    face = query_font_face("serif", MB_FONT_SLANT_ROMAN, 100);
    CU_ASSERT(face != NULL);
    
    blks[0].n_chars = 5;
    blks[0].face = face;
    blks[0].font_sz = 10;
    
    blks[1].n_chars = 4;
    blks[1].face = face;
    blks[1].font_sz = 15.5;
    
    r = sh_stext_set_style((shape_t *)txt_o, blks, 2);
    CU_ASSERT(r == OK);

    compute_styled_extents_n_scaled_font(txt_o);

    ext = &txt_o->extents;
    CU_ASSERT(MBE_GET_HEIGHT(ext) > 47);
    CU_ASSERT(MBE_GET_HEIGHT(ext) < 92);
    CU_ASSERT(MBE_GET_WIDTH(ext) > 36);
    CU_ASSERT(MBE_GET_WIDTH(ext) < 72);
    CU_ASSERT(MBE_GET_X_ADV(ext) > 36);
    CU_ASSERT(MBE_GET_X_ADV(ext) < 72);
    CU_ASSERT(MBE_GET_Y_ADV(ext) > 36);
    CU_ASSERT(MBE_GET_Y_ADV(ext) < 72);
    
    _rdman_shape_stext_free((shape_t *)txt_o);
    free_font_face(face);
}

static
void test_sh_stext_transform(void) {
    sh_stext_t *txt_o;
    mb_style_blk_t blks[2];
    co_aix *aggr;
    mb_font_face_t *face;
    area_t *area;
    int r;

    txt_o = (sh_stext_t *)rdman_shape_stext_new(NULL, "hello world", 100, 50);
    CU_ASSERT(txt_o != NULL);

    aggr = txt_o->shape.aggr;
    aggr[0] = 2;
    aggr[1] = 0;
    aggr[2] = 0;
    aggr[3] = 0;
    aggr[4] = 1;
    aggr[5] = 0;

    face = query_font_face("serif", MB_FONT_SLANT_ROMAN, 100);
    CU_ASSERT(face != NULL);
    
    blks[0].n_chars = 5;
    blks[0].face = face;
    blks[0].font_sz = 10;
    
    blks[1].n_chars = 4;
    blks[1].face = face;
    blks[1].font_sz = 15.5;
    
    r = sh_stext_set_style((shape_t *)txt_o, blks, 2);
    CU_ASSERT(r == OK);

    sh_stext_transform((shape_t *)txt_o);

    area = sh_get_area((shape_t *)txt_o);
    CU_ASSERT(area->x >= 200 && area->x < 220);
    CU_ASSERT(area->y >= 40 && area->y < 50);
    CU_ASSERT(area->w >= 80 && area->w < 120);
    CU_ASSERT(area->h >= 8 && area->h < 12);
    
    _rdman_shape_stext_free((shape_t *)txt_o);
    free_font_face(face);
}

static
void test_sh_stext_draw(void) {
    sh_stext_t *txt_o;
    mb_style_blk_t blks[2];
    co_aix *aggr;
    mb_font_face_t *face;
    area_t *area;
    int r;

    txt_o = (sh_stext_t *)rdman_shape_stext_new(NULL, "hello world", 100, 50);
    CU_ASSERT(txt_o != NULL);
    
    aggr = txt_o->shape.aggr;
    aggr[0] = 2;
    aggr[1] = 0;
    aggr[2] = 0;
    aggr[3] = 0;
    aggr[4] = 1;
    aggr[5] = 0;

    face = query_font_face("serif", MB_FONT_SLANT_ROMAN, 100);
    CU_ASSERT(face != NULL);
    
    blks[0].n_chars = 5;
    blks[0].face = face;
    blks[0].font_sz = 10;
    
    blks[1].n_chars = 6;
    blks[1].face = face;
    blks[1].font_sz = 15.5;
    
    r = sh_stext_set_style((shape_t *)txt_o, blks, 2);
    CU_ASSERT(r == OK);

    sh_stext_transform((shape_t *)txt_o);

    move_cnt = 0;
    sh_stext_draw((shape_t *)txt_o, NULL);
    CU_ASSERT(move_cnt == 2);
    CU_ASSERT(move_xys[0][0] == 200);
    CU_ASSERT(move_xys[0][1] == 50);
    CU_ASSERT(move_xys[1][0] >= 240 && move_xys[1][0] < 270);
    CU_ASSERT(move_xys[1][1] == 50);

    _rdman_shape_stext_free((shape_t *)txt_o);
    free_font_face(face);
}

static
void test_sh_stext_draw_rotate(void) {
    sh_stext_t *txt_o;
    mb_style_blk_t blks[2];
    co_aix *aggr;
    mb_font_face_t *face;
    area_t *area;
    int r;

    txt_o = (sh_stext_t *)rdman_shape_stext_new(NULL, "hello world", 100, 50);
    CU_ASSERT(txt_o != NULL);
    
    aggr = txt_o->shape.aggr;
    aggr[0] = 2;
    aggr[1] = 0;
    aggr[2] = 0;
    aggr[3] = 1;
    aggr[4] = 1;
    aggr[5] = 0;

    face = query_font_face("serif", MB_FONT_SLANT_ROMAN, 100);
    CU_ASSERT(face != NULL);
    
    blks[0].n_chars = 5;
    blks[0].face = face;
    blks[0].font_sz = 10;
    
    blks[1].n_chars = 6;
    blks[1].face = face;
    blks[1].font_sz = 15.5;
    
    r = sh_stext_set_style((shape_t *)txt_o, blks, 2);
    CU_ASSERT(r == OK);

    sh_stext_transform((shape_t *)txt_o);

    move_cnt = 0;
    sh_stext_draw((shape_t *)txt_o, NULL);
    CU_ASSERT(move_cnt == 2);
    CU_ASSERT(move_xys[0][0] == 200);
    CU_ASSERT(move_xys[0][1] == 150);
    CU_ASSERT(move_xys[1][0] >= 240 && move_xys[1][0] < 270);
    CU_ASSERT(move_xys[1][1] >= 170 && move_xys[1][1] < 185);

    _rdman_shape_stext_free((shape_t *)txt_o);
    free_font_face(face);
}

#include <CUnit/Basic.h>
CU_pSuite get_stext_suite(void) {
    CU_pSuite suite;

    suite = CU_add_suite("Suite_stext", NULL, NULL);
    CU_ADD_TEST(suite, test_query_font_face);
    CU_ADD_TEST(suite, test_make_scaled_font_face_matrix);
    CU_ADD_TEST(suite, test_compute_text_extents);
    CU_ADD_TEST(suite, test_extent_extents);
    CU_ADD_TEST(suite, test_compute_utf8_chars_sz);
    CU_ADD_TEST(suite, test_compute_styled_extents_n_scaled_font);
    CU_ADD_TEST(suite, test_compute_styled_extents_n_scaled_font_rotate);
    CU_ADD_TEST(suite, test_sh_stext_transform);
    CU_ADD_TEST(suite, test_sh_stext_draw);
    CU_ADD_TEST(suite, test_sh_stext_draw_rotate);

    return suite;
}

#endif /* UNITTEST */
