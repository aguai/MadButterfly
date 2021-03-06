package org.madbutterfly;

public class shape {
    protected int addr;
    redraw_man rdman;
    protected coord parent;
    protected paint stroke, fill;

    public shape(redraw_man rdman, int addr) {
	this.addr = addr;
	this.rdman = rdman;
	this.parent = null;
	stroke = fill = null;
    }
    
    protected void invalid() {
	addr = 0;
    }
    
    protected void finalize() {
	if(addr != 0)
	    _jni.rdman_shape_free(rdman._rdman_addr, addr);
    }

    public void set_stroke_width(float w) {
	_jni.sh_set_stroke_width(addr, w);
    }
}
