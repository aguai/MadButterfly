#include <SkCanvas.h>
#include <SkBitmap.h>

extern "C" {
#include <mb.h>
};

void native_test(void) {
    mbe_t *mbe1, *mbe2;
    SkBitmap *bmap1, *bmap2;
    redraw_man_t _rdman;
    redraw_man_t *rdman = &_rdman;
    coord_t *root;
    shape_t *shape;
    paint_t *paint;

    bmap1 = new SkBitmap();
    bmap1->setConfig(SkBitmap::kARGB_8888_Config, 300, 300);
    mbe1 = mbe_create((mbe_surface_t *)bmap1);
    bmap2 = new SkBitmap();
    bmap2->setConfig(SkBitmap::kARGB_8888_Config, 300, 300);
    mbe2 = mbe_create((mbe_surface_t *)bmap2);

    redraw_man_init(rdman, mbe1, mbe2);
    root = rdman_get_root(rdman);
    shape = rdman_shape_path_new(rdman, "M 100 100 L 100 150 L 150 150 z");
    paint = rdman_paint_color_new(rdman, 1, 0, 0, 1);
    rdman_paint_stroke(rdman, paint, shape);
    rdman_add_shape(rdman, shape, root);

    rdman_shape_changed(rdman, shape);
    rdman_paint_changed(rdman, paint);

    rdman_redraw_all(rdman);

    rdman_paint_free(rdman, paint);
    rdman_shape_free(rdman, shape);
    redraw_man_destroy(rdman);
}

int main(int argc, const char *argv[]) {
    native_test();
    return 0;
}
