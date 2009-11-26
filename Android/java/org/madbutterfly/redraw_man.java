package org.madbutterfly;

import android.graphics.Canvas;
import android.view.SurfaceHolder;
import java.util.ArrayList;

class redraw_man {
    Canvas _cr, _backend;
    int _rdman_addr;		// address of redraw_man_t
    coord root;
    ArrayList paints;
    MBView _view;
    
    public redraw_man(Canvas cr, Canvas backend, MBView view) {
	int addr;
	
	_view = view;
	_cr = cr;
	_backend = backend;
	
	_rdman_addr = _jni.redraw_man_new(cr, backend);
	addr = _jni.rdman_get_root(_rdman_addr);
	root = new coord(this, addr);
	
	paints = new ArrayList();
    }

    static void _invalid_subtree(coord subtree) {
	int i, sz;
	coord child;
	shape member;

	subtree.invalid();
	
	sz = subtree.members.size();
	for(i = 0; i < sz; i ++) {
	    member = (shape)subtree.members.get(i);
	    member.invalid();
	}

	sz = subtree.children.size();
	for(i = 0; i < sz; i++) {
	    child = (coord)subtree.children.get(i);
	    _invalid_subtree(child);
	}
    }

    protected void finalize() {
	int i, sz;
	paint pnt;
	
	_invalid_subtree(root);
	
	/* invalid paints */
	sz = paints.size();
	for(i = 0; i < sz; i++) {
	    pnt = (paint)paints.get(i);
	    pnt.invalid();
	}
	
	_jni.redraw_man_free(_rdman_addr);
    }

    public coord get_root() {
	return root;
    }

    public coord coord_new(coord parent) {
	int addr;
	coord child;
	
	addr = _jni.rdman_coord_new(_rdman_addr, parent.addr);
	child = new coord(this, addr);
	parent.children.add(child);
	
	return child;
    }

    public void coord_free(coord obj) {
	obj.finalize();
	obj.invalid();
    }

    public shape shape_path_new(String pathdata) {
	int addr;
	shape new_shape;

	addr = _jni.rdman_shape_path_new(_rdman_addr, pathdata);
	new_shape = new shape(this, addr);

	return new_shape;
    }

    public paint paint_color_new(float r, float g, float b, float a) {
	int addr;
	paint pnt;

	addr = _jni.rdman_paint_color_new(_rdman_addr, r, g, b, a);
	pnt = new paint(this, addr);
	
	return pnt;
    }

    public void redraw() {
	_jni.rdman_redraw_all(_rdman_addr);
	_view.redraw();
    }
}
