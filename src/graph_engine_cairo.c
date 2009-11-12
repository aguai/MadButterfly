#include <fontconfig/fontconfig.h>
#include "mb_graph_engine_cairo.h"
#include "mb_shapes.h"

#ifndef ASSERT
#define ASSERT(x)
#endif

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
mbe_font_face_t *
mbe_query_font_face(const char *family, int slant, int weight) {
    mbe_font_face_t *cface;
    FcPattern *ptn;
    
    ptn = query_font_pattern(family, slant, weight);
    cface = cairo_ft_font_face_create_for_pattern(ptn);
    FcPatternDestroy(ptn);
    
    return cface;
}

void
mbe_free_font_face(mbe_font_face_t *face) {
    ASSERT(face == NULL);

    mbe_font_face_destroy(face);
}

