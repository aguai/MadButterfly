#include <stdio.h>
#include <cairo.h>
#include <fontconfig.h>
#include "mb_shapes.h"

#if 0

#ifndef ASSERT
#define ASSERT(x)
#endif
#define OK 0
#define ERR -1

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
    cairo_font_face_t *cface;
    FcPattern *ptn;
    FcValue val;
    static int slant_map[] = {	/* from MB_FONT_SLANT_* to FC_SLANT_* */
	FC_SLANT_ROMAN,
	FC_SLANT_ROMAN,
	FC_SLANT_ITALIC,
	FC_SLANT_OBLIQUE};

    ptn = FcPatternCreate();
    val.type = FcTypeString;
    val.s = family;
    FcPatternAdd(ptn, "family", val, FcTrue);
    
    if(family < 0 || family >= MB_FONT_SLANT_MAX) {
	FcPatternDestroy(ptn);
	return NULL;
    }
    val.type = FcTypeInteger;
    val.i = slant_map[slant];
    FcPatternAdd(ptn, "slant", val, FcTrue);
    
    val.type = FcTypeInteger;
    val.i = weight;
    FcPatternAdd(ptn, "weight", val, FcTrue);

    cface = cairo_ft_font_face_create_for_pattern(ptn);
    FcPatternDestroy(ptn);
    
    return (mb_font_face_t *)cface;
}

void mb_font_face_free(mb_font_face_t *face) {
    cairo_font_face_t *cface = (cairo_font_face_t *)face;
    ASSERT(face != NULL);
    cairo_font_face_destroy(face);
}

/*! \brief A simple implementation of text shape.
 *
 */
typedef struct _sh_stext {
    shape_t shape;
    const char *txt;		      /*!< \brief Text to be showed */
    const mb_style_blk_t *style_blks; /*!< \brief Style of text */
    int nblks;			      /*!< \brief Number of style blocks */
    int max_nblks;
    co_aix x, y;
    cairo_scaled_font_t **scaled_fonts;
} sh_stext_t;

shape_t *rdman_shape_stext_new(redraw_man_t *rdman, co_aix x, co_aix y,
		      const char *txt) {
    sh_stext_t *txt_o;

    ASSERT(txt != NULL);
    
    txt_o = (sh_stext_t *)malloc(sizeof(sh_stext_t));
    if(txt_o == NULL)
	return NULL;

    memset(&txt_o->shape, 0, sizeof(shape_t));
    mb_obj_init(txt_o, MBO_STEXT);
    
    txt_o->txt = strdup(txt);
    txt_o->style_blks = NULL;
    txt_o->nblks = 0;
    txt_o->max_nblks = 0;
    txt_o->x = x;
    txt_o->y = y;
    txt_o->exts = NULL;

    if(txt_o->txt == NULL) {
	free(txt_o);
	txt_o = NULL;
    }

    return (shape_t *)txt_o;
}

static
int compute_utf8_chars_sz(const char *txt, int n_chars) {
    int i;
    const char *p = txt;
    
    for(i = 0; i < n && *p; i++) {
	if(*p++ & 0x80)
	    continue;		/* single byte */
	/* multi-bytes */
	while(*p && ((*p & 0xc0) == 0x80))
	    p++;
    }
    if(i < n)
	return ERR;
    
    return p - txt;
}

static
void compute_text_extents(char *txt, int txt_len,
			 cairo_scaled_font_t *scaled_font,
			 cairo_text_extents_t *extents) {
    char saved;

    saved = txt[txt_len];
    txt[txt_len] = 0;
    cairo_scaled_font_text_extents(scaled_font, txt, extents);
    txt[txt_len] = saved;
}

static
cairo_scaled_font_t *make_scaled_font_face(sh_stext_t *txt_o,
					   cairo_font_face_t *face,
					   co_aix shift_x, co_aix shift_y,
					   co_aix font_sz) {
    cairo_matrix_t cmtx;
    static cairo_matrix_t cid;
    static cairo_font_options_t *fopt = NULL;
    co_aix *matrix;
    cairo_scaled_font_t *scaled;
    int i;

    if(fopt == NULL) {
	cairo_matrix_init_identify(&cid);
	fopt = cairo_font_options_create();
	if(fopt == NULL)
	    return NULL;
    }

    matrix = sh_get_aggr_matrix((shape_t *)txt_o);
    i = 0;
    cmtx.xx = matrix[i++];
    cmtx.xy = matrix[i++];
    cmtx.x0 = matrix[i++] + shift_x;
    cmtx.yx = matrix[i++];
    cmtx.yy = matrix[i++];
    cmtx.y0 = matrix[i] + shift_y;
    
    scaled = cairo_scaled_font_create(face, &cmtx, &cid, fopt);
    
    return scaled;
}

static
void compute_extents(sh_stext_t *txt_o) {
    co_aix adv_x, adv_y;
    cairo_text_extents_t extents;
    mb_style_blk_t *blk;
    cairo_scaled_font_t *scaled_font;
    char *txt;
    co_aix shift_x, shift_y;
    int blk_txt_len;
    int i;

    adv_x = adv_y = 0;
    blk = txt_o->style_blks;
    scaled_font = txt_o->scaled_fonts;
    txt = (char *)txt_o->txt;
    shift_x = txt_o->x;
    shift_y = txt_o->y;
    for(i = 0; i < txt_o->nblks; i++) {
	*scaled_font = make_scaled_fonts(txt_o, blk->face,
					 shift_x, shift_y, font_sz);
	ASSERT(*scaled_font != NULL);
	
	blk_txt_len = compute_utf8_chars_sz(txt, blk->n_chars);
	ASSERT(blk_txt_len != ERR);
	
	compute_text_extents(txt, blk_txt_len, *scaled_font, &extents);
	adv_x += extents.x_advance;
	adv_y += extents.y_advance;
	
	scaled_font++;
	blk++;
	txt += blk_txt_len;
    }
}

/*
 * What we have to do in sh_stext_transform() is
 * - computing bounding box for the text,
 * - computing offset x,y for the text of style blocks,
 * - free old scaled fonts, and
 * - making scaled fonts for style blocks.
 */
void sh_stext_transform(shape_t *shape) {
    sh_stext_t *txt_o = (sh_stext_t *)shape;

    ASSERT(txt_o != NULL);
}

void sh_stext_draw(shape_t *shape, cairo_t *cr) {
    sh_stext_t *txt_o = (sh_stext_t *)shape;

    ASSERT(txt_o != NULL);
}

int sh_stext_set_text(shape_t *shape, const char *txt) {
    sh_stext_t *txt_o = (sh_stext_t *)shape;
    char *new_txt;
    int sz;

    ASSERT(txt_o != NULL);
    ASSERT(txt != NULL);
    
    sz = strlen(txt) + 1;
    new_txt = realloc(txt_o->txt, sz);
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
    mb_style_blk_t *new_blks;
    co_aix *new_exts;
    int sz;

    ASSERT(txt_o != NULL);
    ASSERT(nblks >= 0);
    
    if(nblks > txt_o->max_nblks) {
	sz = nblks * (sizeof(mb_style_blk_t) + sizeof(int));
	new_blks = (mb_style_blk_t *)realloc(txt_o->style_blks, sz);
	if(new_blks == NULL)
	    return ERR;
	new_exts = (int *)(new_blks + nblks);
	
	txt_o->style_blks = new_blks;
	txt_o->exts = new_exts;
	txt_o->max_nblks = nblks;
    }

    memcpy(txt_o->blks, blks, nblks * sizeof(mb_style_blk_t));
    txt_o->nblks = nblks;
    
    return OK;
}

#endif /* 0 */
