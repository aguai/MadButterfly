package org.madbutterfly.android.examples.testpath;

import android.app.Activity;
import android.view.SurfaceHolder;
import android.content.Context;
import android.os.Bundle;
import android.graphics.Canvas;
import android.util.Log;
import org.madbutterfly.MBView;
import org.madbutterfly.redraw_man;
import org.madbutterfly.coord;
import org.madbutterfly.shape;
import org.madbutterfly.paint;

public class testpath extends Activity implements SurfaceHolder.Callback
{
    MBView main_view;
    String TAG = "testpath";

    /** Called when the activity is first created. */
    @Override
    public void onCreate(Bundle savedInstanceState)
    {
        super.onCreate(savedInstanceState);
	SurfaceHolder holder;
	
        // setContentView(R.layout.main);
	main_view = new MBView(this);
	holder = main_view.getHolder();
	holder.addCallback(this);
	setContentView(main_view);
    }

    public void surfaceChanged(SurfaceHolder holder, int format,
			       int w, int h) {
    }

    public void surfaceCreated(SurfaceHolder holder) {
	redraw_man rdman;
	shape path;
	coord _coord;
	paint pnt;
	
	Log.v(TAG, "surfaceCreated enter");
	rdman = main_view.get_rdman();
	path = rdman.shape_path_new("M 100 100 L 150 100 L 150 150 z");
	path.set_stroke_width(2);
	_coord = rdman.get_root();
	_coord.add_shape(path);
	pnt = rdman.paint_color_new(1, 0, 1, 1);
	pnt.stroke(path);
	Log.v(TAG, "surfaceCreated before redraw");
	rdman.redraw();
	Log.v(TAG, "surfaceCreated leave");
    }

    public void surfaceDestroyed(SurfaceHolder holder) {
    }
}
