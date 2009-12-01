LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

LOCAL_MODULE := mbfly-java
LOCAL_MODULE_CLASS := JAVA_LIBRARIES

LOCAL_SRC_FILES := \
	org/madbutterfly/_jni.java \
	org/madbutterfly/MBView.java \
	org/madbutterfly/redraw_man.java \
	org/madbutterfly/coord.java \
	org/madbutterfly/shape.java \
	org/madbutterfly/paint.java \
	org/madbutterfly/InvalidStateException.java

include $(BUILD_JAVA_LIBRARY)

MBFLY_JAVA:= $(strip $(intermediates))/classes.jar
MADBUTTERFLY_ATREE:= $(strip $(LOCAL_PATH))/madbutterfly.atree

ALL_SDK_FILES += $(MBFLY_JAVA)

ADDITIONAL_ATREE_FILES += $(MADBUTTERFLY_ATREE)
