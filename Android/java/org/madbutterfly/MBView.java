package org.madbutterfly;

import android.view.SurfaceView;
import android.view.SurfaceHolder;
import android.content.Context;
import android.graphics.Canvas;
import android.graphics.Bitmap;
import android.graphics.Paint;
import android.graphics.Xfermode;
import android.graphics.PorterDuff;
import android.graphics.PorterDuffXfermode;

public class MBView extends SurfaceView {
    redraw_man rdman;
    Canvas cr, backend;
    Bitmap cr_bmap, backend_bmap;
    Paint copy_pnt;
    int w, h;
    
    public MBView(Context context) {
	super(context);
	Paint paint;
	Xfermode mode;
	
	rdman = null;
	cr = null;
	backend = null;
	
	mode = new PorterDuffXfermode(PorterDuff.Mode.SRC);
	copy_pnt = new Paint();
	copy_pnt.setXfermode(mode);
	w = 100;
	h = 100;
    }
    
    public redraw_man get_rdman() {
	if(rdman != null)
	    return rdman;

	w = getWidth();
	h = getHeight();

	cr_bmap = Bitmap.createBitmap(w, h, Bitmap.Config.ARGB_8888);
	cr = new Canvas(cr_bmap);
	backend_bmap = Bitmap.createBitmap(w, h, Bitmap.Config.ARGB_8888);
	backend = new Canvas(backend_bmap);
	
	rdman = new redraw_man(cr, backend, this);

	return rdman;
    }

    protected void onSizeChanged(int w, int h, int oldw, int oldh) {
	this.w = w;
	this.h = h;

	if(rdman == null) {
	    get_rdman();
	    return;
	}
	
	cr_bmap = Bitmap.createBitmap(w, h, Bitmap.Config.ARGB_8888);
	cr.setBitmap(cr_bmap);
	backend_bmap = Bitmap.createBitmap(w, h, Bitmap.Config.ARGB_8888);
	backend.setBitmap(backend_bmap);
    }

    public void redraw() {
	SurfaceHolder holder;
	Canvas canvas;

	holder = getHolder();
	canvas = holder.lockCanvas();
	canvas.drawBitmap(backend_bmap, 0, 0, copy_pnt);
	holder.unlockCanvasAndPost(canvas);
    }
}
