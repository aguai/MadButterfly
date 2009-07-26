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
 * It is actually a cairo_scaled_font_t, now.  But, it should not be
 * noticed by out-siders.  Only \ref fontconfig_freetype
 * should known it.
 */
typedef struct _mb_scaled_font mb_scaled_font_t;

/*! \brief Stakeholder of scaled font.
 *
 * Although, mb_text_extents_t is defined as a cairo_scaled_font_t, but
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
typedef cairo_text_extents_t mb_text_extents_t;

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

/*! \brief Find out a font pattern.
 *
 * This function use fontconfig to decide a font file in pattern.  It can
 * replaced by other mechanism if you think it is not what you want.
 *
 * \param slant make font prune if it it non-zero.
 * \param weight make font normal if it is 100.
 */
static
FcPattern *query_font_pattern(const char *family, int slant, int weight) {
    FcPattern *ptn, *p, *fn_ptn;
    FcValue val;
    FcConfig *cfg;
    FcBool r;
    FcResult result;
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
    FcPatternAdd(ptn, "family", val, FcTrue);
    
    val.type = FcTypeInteger;
    val.u.i = slant_map[slant];
    FcPatternAdd(ptn, "slant", val, FcTrue);
    
    val.type = FcTypeInteger;
    val.u.i = weight;
    FcPatternAdd(ptn, "weight", val, FcTrue);

    r = FcConfigSubstituteWithPat(cfg, ptn, NULL, FcMatchPattern);
    if(!r)
	goto err;
    
    r = FcConfigSubstituteWithPat(cfg, p, ptn, FcMatchFont);
    if(!r)
	goto err;

    FcDefaultSubstitute(p);

    fn_ptn = FcFontMatch(cfg, p, &result);

    /* It is supposed to return FcResultMatch.  But, it is no, now.
     * I don't know why.  Someone should figure out the issue.
     */
#if 0
    if(result != FcResultMatch) {
	printf("%d %d\n", result, FcResultMatch);
	goto err;
    }
#endif
    if(fn_ptn == NULL)
	goto err;

    FcPatternDestroy(ptn);
    FcPatternDestroy(p);
    
    return fn_ptn;
    
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
mb_scaled_font_t *make_scaled_font_face_matrix(mb_font_face_t *face,
					       co_aix *matrix) {
    cairo_scaled_font_t *scaled_font;
    cairo_matrix_t font_matrix;
    static cairo_matrix_t id = {
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
void scaled_font_free(mb_scaled_font_t *scaled_font) {
    cairo_scaled_font_destroy((cairo_scaled_font_t *)scaled_font);
}

static
void compute_text_extents(mb_scaled_font_t *scaled_font, const char *txt,
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
    int nblks;			      /*!< \brief Number of valid style
				       *    blocks */
    int max_nblks;		      /*!< \brief Available space of
				       *    style_blks */
    co_aix x, y;
    mb_scaled_font_t **scaled_fonts;
    int nscaled;
    int maxscaled;
    mb_text_extents_t extents;
} sh_stext_t;

static
void _rdman_shape_stext_free(shape_t *shape) {
    sh_stext_t *txt_o = (sh_stext_t *)shape;
    int i;

    if(txt_o->style_blks)
	free((void *)txt_o->style_blks);
    if(txt_o->scaled_fonts) {
	for(i = 0; i < txt_o->nscaled; i++)
	    scaled_font_free(txt_o->scaled_fonts[i]);
	free(txt_o->scaled_fonts);
    }
    if(txt_o->txt)
	free((void *)txt_o->txt);

    free(txt_o);
}

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
    txt_o->scaled_fonts = NULL;
    txt_o->nscaled = 0;
    txt_o->maxscaled = 0;

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
    mb_scaled_font_t *scaled;

    aggr = sh_get_aggr_matrix((shape_t *)txt_o);
    matrix[0] = font_sz;
    matrix[1] = 0;
    matrix[2] = shift_x;
    matrix[3] = 0;
    matrix[4] = font_sz;
    matrix[5] += shift_y;
    matrix_mul(aggr, matrix, scaled_matrix);
    
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
    co_aix f_rbx, f_rby;	/* rb stands for right button */
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
 */
static
void compute_styled_extents_n_scaled_font(sh_stext_t *txt_o) {
    mb_text_extents_t sub_extents;
    const mb_style_blk_t *blk;
    int blk_txt_len;
    mb_scaled_font_t **scaled_font;
    char *txt, saved;
    co_aix shift_x, shift_y;
    int i;
    
    memset(&txt_o->extents, sizeof(mb_text_extents_t), 0);
    
    blk = txt_o->style_blks;
    scaled_font = txt_o->scaled_fonts;
    txt = (char *)txt_o->txt;
    for(i = 0; i < txt_o->nblks; i++) {
	shift_x = txt_o->x + MBE_GET_X_ADV(&txt_o->extents);
	shift_y = txt_o->y + MBE_GET_Y_ADV(&txt_o->extents);
	
	*scaled_font = make_scaled_font_face(txt_o, blk->face,
					     shift_x, shift_y, blk->font_sz);
	ASSERT(*scaled_font != NULL);
	
	blk_txt_len = compute_utf8_chars_sz(txt, blk->n_chars);
	ASSERT(blk_txt_len != ERR);
	
	saved = txt[blk_txt_len];
	txt[blk_txt_len] = 0;
	compute_text_extents(*scaled_font, txt, &sub_extents);
	txt[blk_txt_len] = saved;

	extent_extents(&txt_o->extents, &sub_extents);

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
    compute_styled_extents_n_scaled_font(txt_o);
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
    mb_style_blk_t *new_blks;
    int sz;

    ASSERT(txt_o != NULL);
    ASSERT(nblks >= 0);
    
    if(nblks > txt_o->max_nblks) {
	sz = nblks * sizeof(mb_style_blk_t);
	new_blks = (mb_style_blk_t *)realloc((void *)txt_o->style_blks, sz);
	if(new_blks == NULL)
	    return ERR;
	
	txt_o->style_blks = new_blks;
	txt_o->max_nblks = nblks;
    }

    memcpy(txt_o->style_blks, blks, nblks * sizeof(mb_style_blk_t));
    txt_o->nblks = nblks;
    
    return OK;
}

#ifdef UNITTEST

#include <CUnit/Basic.h>

static
void test_query_font_face(void) {
    mb_font_face_t *face;
    cairo_status_t status;

    face = query_font_face("serif", MB_FONT_SLANT_ROMAN, 100);
    CU_ASSERT(face != NULL);
    status = cairo_font_face_status(face);
    CU_ASSERT(status == CAIRO_STATUS_SUCCESS);
    
    free_font_face(face);
}

static
void test_make_scaled_font_face_matrix(void) {
    co_aix matrix[6] = {5, 0, 0, 0, 5, 0};
    mb_font_face_t *face;
    mb_scaled_font_t *scaled;
    cairo_status_t status;

    face = query_font_face("serif", MB_FONT_SLANT_ROMAN, 100);
    CU_ASSERT(face != NULL);
    status = cairo_font_face_status(face);
    CU_ASSERT(status == CAIRO_STATUS_SUCCESS);
    
    scaled = make_scaled_font_face_matrix(face, matrix);
    CU_ASSERT(scaled != NULL);
    status = cairo_scaled_font_status((cairo_scaled_font_t *)scaled);
    CU_ASSERT(status == CAIRO_STATUS_SUCCESS);
    
    scaled_font_free(scaled);
    free_font_face(face);
}

static
void test_compute_text_extents(void) {
    co_aix matrix[6] = {10, 0, 0, 0, 10, 0};
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

    scaled_font_free(scaled);
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

#include <CUnit/Basic.h>
CU_pSuite get_stext_suite(void) {
    CU_pSuite suite;

    suite = CU_add_suite("Suite_stext", NULL, NULL);
    CU_ADD_TEST(suite, test_query_font_face);
    CU_ADD_TEST(suite, test_make_scaled_font_face_matrix);
    CU_ADD_TEST(suite, test_compute_text_extents);
    CU_ADD_TEST(suite, test_extent_extents);
    CU_ADD_TEST(suite, test_compute_utf8_chars_sz);

    return suite;
}

#endif /* UNITTEST */
