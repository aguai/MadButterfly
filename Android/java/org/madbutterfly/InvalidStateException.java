package org.madbutterfly;

import java.lang.Exception;

class InvalidStateException extends Exception {
    public InvalidStateException() {
	super();
    }
    public InvalidStateException(String msg) {
	super(msg);
    }
}
