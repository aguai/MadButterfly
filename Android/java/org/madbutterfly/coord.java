package org.madbutterfly;

import java.util.ArrayList;

public class coord {
    protected int addr;
    redraw_man rdman;
    public ArrayList children;
    public ArrayList members;

    public coord(redraw_man rdman, int addr) {
	this.addr = addr;
	this.rdman = rdman;
	children = new ArrayList();
	members = new ArrayList();
    }

    protected void invalid() {
	addr = 0;
    }

    protected void finalize() {
	if(addr != 0)
	    _jni.rdman_coord_free(rdman._rdman_addr, addr);
    }

    public void add_shape(shape member) {
	_jni.rdman_add_shape(rdman._rdman_addr, member.addr, addr);
	members.add(member);
	member.parent = this;
    }
}
