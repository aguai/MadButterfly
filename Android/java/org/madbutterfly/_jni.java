package org.madbutterfly;
import android.graphics.Canvas;

class _jni {
    native static int redraw_man_new(Canvas cr, Canvas backend);
    native static void redraw_man_free(int rdman);
    native static int rdman_add_shape(int rdman, int shape, int coord);
    native static int rdman_get_root(int rdman);
    native static int rdman_redraw_all(int rdman);
    native static int rdman_redraw_changed(int rdman);
    native static int rdman_redraw_area(int rdman, float x, float y,
					float w, float h);
    native static void rdman_paint_fill(int rdman, int paint, int shape);
    native static void rdman_paint_stroke(int rdman, int paint, int shape);
    
    /* coord_t */
    native static int rdman_coord_new(int rdman, int parent);
    native static void rdman_coord_free(int rdman, int coord);
    native static void rdman_coord_subtree_free(int rdman, int coord);
    native static void rdman_coord_changed(int rdman, int coord);
    
    /* shape_t */
    native static void rdman_shape_changed(int rdman, int shape);
    native static void rdman_shape_free(int rdman, int shape);
    native static int rdman_shape_path_new(int rdman, String data);
    native static void sh_set_stroke_width(int shape, float w);

    /* paint_t */
    native static int rdman_paint_color_new(int rdman, float r, float g,
					    float b, float a);
    native static int rdman_paint_free(int rdman, int paint);
    native static void paint_color_set(int paint, float r, float g,
				      float b, float a);
    native static float[] paint_color_get(int paint);
    native static int rdman_paint_linear_new(int rdman,
					     float x1, float y1,
					     float x2, float y2);
    native static int paint_linear_stops(int paint, int n_stops, int stops);
    native static int rdman_paint_radial_new(int rdman, float cx, float cy,
					     float r);
    native static int paint_radial_stops(int paint, int n_stops, int stops);
    native static int paint_create_stops(float stops[][]);
    native static int paint_free_stops(int stops);
    
    static {
	System.loadLibrary("mbfly-jni");
    }
}
