#include "mb_graph_engine_openvg.h"

EGLNativeDisplayType _ge_openvg_disp_id = EGL_DEFAULT_DISPLAY;
mbe_t *_ge_openvg_current_canvas = NULL;

void
mbe_scissoring(mbe_t *canvas, int n_areas, area_t **areas) {
    static VGint *coords = NULL;
    static int coords_sz = 0;
    VGint *coord;
    area_t *area;
    int i;

    _MK_CURRENT_CTX(canvas);

    if(n_areas > coords_sz) {
	if(coords) free(coords);
	coords_sz = (n_areas + 0xf) & 0xf;
	coords = (VGint *)malloc(sizeof(VGint) * coords_sz * 4);
	ASSERT(coords != NULL);
    }
    
    coord = coords;
    for(i = 0; i < n_areas; i++) {
	area = areas[i];
	*coord++ = area->x;
	*coord++ = area->y;
	*coord++ = area->w;
	*coord++ = area->h;
    }

    vgSetiv(VG_SCISSOR_RECTS, n_areas * 4, coords);
}
