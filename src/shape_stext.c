#ifdef SHAPE_STEXT

#include <stdio.h>
#include <cairo.h>
#include <cairo-ft.h>
#include <fontconfig/fontconfig.h>
#include "mb_shapes.h"

#ifndef ASSERT
#define ASSERT(x)
#endif
#define OK 0
#define ERR -1

/*! \defgroup fontconfig_freetype Fontconfig and FreeType Layer.
 *
 * This layer implements a font provider to reset of the system.
 * It bases on fontconfig and FreeType supporting of Cairo.
 *
 * @{
 */
/*! \brief Stakeholder of scaled font.
 *
 * It is actually a cairo_scaled_font_t, now.  But, it should not be
 * noticed by out-siders.  Only \ref fontconfig_freetype
 * should known it.
 */
typedef struct _mb_scaled_font mb_scaled_font_t;

/*! \brief Stakeholder of scaled font.
 *
 * Although, mb_text_extents_t is defined as a cairo_scaled_font_t, but
 * programmers should assume it is opague.
 */
typedef cairo_text_extents_t mb_text_extents_t;

#define MBE_GET_X_ADV(ext) ((ext)->x_advance)
#define MBE_GET_Y_ADV(ext) ((ext)->y_advance)
#define MBE_GET_WIDTH(ext) ((ext)->width)
#define MBE_GET_HEIGHT(ext) ((ext)->height)

/*! \brief Find out a font pattern.
 *
 * This function use fontconfig to decide a font file in pattern.  It can
 * replaced by other mechanism if you think it is not what you want.
 */
static
FcPattern *query_font_pattern(const char *family, int slant, int weight) {
    FcPattern *ptn, *p;
    FcValue val;
    FcConfig *cfg;
    FcBool r;
    static int slant_map[] = {	/* from MB_FONT_SLANT_* to FC_SLANT_* */
	FC_SLANT_ROMAN,
	FC_SLANT_ROMAN,
	FC_SLANT_ITALIC,
	FC_SLANT_OBLIQUE};

    cfg = FcConfigGetCurrent();
    ptn = FcPatternCreate();
    p = FcPatternCreate();
    if(ptn == NULL || p == NULL)
	goto err;

    val.type = FcTypeString;
    val.u.s = family;
    FcPatternAdd(ptn, "family", &val, FcTrue);
    
    val.type = FcTypeInteger;
    val.u.i = slant_map[slant];
    FcPatternAdd(ptn, "slant", &val, FcTrue);
    
    val.type = FcTypeInteger;
    val.u.i = weight;
    FcPatternAdd(ptn, "weight", &val, FcTrue);

    r = FcConfigSubstituteWithPat(cfg, ptn, NULL, FcMatchPattern);
    if(!r)
	goto err;
    
    r = FcConfigSubstituteWithPat(cfg, p, ptn, FcMatchFont);
    if(!r)
	goto err;

    FcDefaultSubstitute(p);

    FcPatternDestroy(ptn);

    return p;
    
err:
    if(ptn)
	FcPatternDestroy(ptn);
    if(p)
	FcPatternDestroy(p);
    return NULL;

}

/*! \brief Find out a font face for a pattern specified.
 *
 * The pattern, here, is a vector of family, slant, and weight.
 * This function base on fontconfig and cairo FreeType font supporting.
 * You can replace this function with other font mechanisms.
 */
static
mb_font_face_t *query_font_face(const char *family, int slant, int weight) {
    cairo_font_face_t *cface;
    FcPattern *ptn;
    
    ptn = query_font_pattern(family, slant, weight);
    cface = cairo_ft_font_face_create_for_pattern(ptn);
    FcPatternDestroy(ptn);
    
    return (mb_font_face_t *)cface;
}

static
void free_font_face(mb_font_face_t *face) {
    ASSERT(face == NULL);

    cairo_font_face_destroy((cairo_font_face_t *)face);
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
mb_scaled_font_t *get_cairo_scaled_font(mb_font_face_t *face,
					co_aix *matrix) {
    cairo_font_face_t *scaled_font;
    cairo_matrix_t font_matrix;
    static cairo_matir_t id = {
	1, 0,
	0, 1,
	0, 0
    };
    static cairo_font_options_t *opt = NULL;

    ASSERT(matrix != NULL);

    if(opt == NULL) {
	opt = cairo_font_options_create();
	if(opt == NULL)
	    return NULL;
    }

    font_matrix.xx = *matrix++;
    font_matrix.xy = *matrix++;
    font_matrix.x0 = *matrix++;
    font_matrix.yx = *matrix++;
    font_matrix.yy = *matrix++;
    font_matrix.y0 = *matrix;
    scaled_font = cairo_scaled_font_create((cairo_font_face_t *)face,
					   &font_matrix,
					   &id, opt);

    return (mb_scaled_font_t *)scaled_font;
}

static
void free_scaled_font(mb_scaled_font_t *scaled_font) {
    cairo_scaled_font_destroy((cairo_scaled_font_t *)scaled_font);
}

static
void text_extents(mb_scaled_font_t *scaled_font, const char *txt,
		  mb_text_extents_t *extents) {
    cairo_scaled_font_text_extents((cairo_scaled_font_t *)scaled_font,
				   txt,
				   (cairo_text_extents_t *)extents);
}

static
mb_text_extents_t *mb_text_extents_new(void) {
    cairo_text_extents_t *extents;

    extents = (cairo_text_extents_t *)malloc(sizeof(cairo_text_extents_t));
    return extents;
}

static
void mb_text_extents_free(mb_text_extents_t *extents) {
    free(extents);
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
    mb_scaled_font_t **scaled_fonts;
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

#endif /* SHAPE_STEXT */
