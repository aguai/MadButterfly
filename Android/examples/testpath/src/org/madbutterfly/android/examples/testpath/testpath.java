package org.madbutterfly.android.examples.testpath;

import android.app.Activity;
import android.content.Context;
import android.os.Bundle;
import org.madbutterfly.MBView;
import org.madbutterfly.redraw_man;

public class testpath extends Activity
{
    class myview extends MBView {
	public myview(Context ctx) {
	    super(ctx);
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
