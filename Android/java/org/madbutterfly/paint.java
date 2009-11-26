package org.madbutterfly;

import java.util.ArrayList;

class paint {
    protected int addr;
    redraw_man rdman;
    ArrayList members;

    public paint(redraw_man rdman, int addr) {
	this.addr = addr;
	this.rdman = rdman;
	members = new ArrayList();
    }

    protected void invalid() {
	addr = 0;
    }

    protected void finalize() {
	if(addr != 0)
	    _jni.rdman_paint_free(rdman._rdman_addr, addr);
    }

    public void stroke(shape sh) {
	no_stroke(sh);
	
	if(sh.fill != this)
	    members.add(sh);

	sh.stroke = this;
	
	_jni.rdman_paint_stroke(rdman._rdman_addr, addr, sh.addr);
    }
    
    public void fill(shape sh) {
	no_fill(sh);
	
	if(sh.stroke != this)
	    members.add(sh);

	sh.fill = this;
	
	_jni.rdman_paint_fill(rdman._rdman_addr, addr, sh.addr);
    }

    public boolean is_empty() {
	return members.size() == 0;
    }

    public static void no_stroke(shape sh) {
	int i;
	
	if(sh.stroke != null && sh.stroke != sh.fill) {
	    i = sh.stroke.members.indexOf(sh);
	    sh.stroke.members.remove(i);
	}
	sh.stroke = null;
    }

    public static void no_fill(shape sh) {
	int i;
	
	if(sh.fill != null && sh.stroke != sh.fill) {
	    i = sh.stroke.members.indexOf(sh);
	    sh.stroke.members.remove(i);
	}
	sh.fill = null;
    }
}
