package org.madbutterfly.android.examples.testpath;

import android.app.Activity;
import android.content.Context;
import android.os.Bundle;
import android.graphics.Canvas;
import org.madbutterfly.MBView;
import org.madbutterfly.redraw_man;
import org.madbutterfly.coord;
import org.madbutterfly.shape;
import org.madbutterfly.paint;

public class testpath extends Activity
{
    class myview extends MBView {
	public myview(Context ctx) {
	    super(ctx);
	}

	protected void onDraw(Canvas canvas) {
	    redraw_man rdman;
	    shape path;
	    coord _coord;
	    paint pnt;

	    rdman = get_rdman();
	    path = rdman.shape_path_new("M 100 100 L 150 100 L 150 150 z");
	    _coord = rdman.get_root();
	    _coord.add_shape(path);
	    pnt = rdman.paint_color_new(1, 1, 1, 1);
	    pnt.stroke(path);
	    rdman.redraw();
	}
    }

    MBView main_view;

    /** Called when the activity is first created. */
    @Override
    public void onCreate(Bundle savedInstanceState)
    {
        super.onCreate(savedInstanceState);
        // setContentView(R.layout.main);
	main_view = new myview(this);
	setContentView(main_view);
    }
}
