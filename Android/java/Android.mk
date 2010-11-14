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

########################
include $(CLEAR_VARS)

LOCAL_MODULE := mbfly-permissions.xml

LOCAL_MODULE_TAGS := user

LOCAL_MODULE_CLASS := ETC

# This will install the file in /system/etc/permissions
#
LOCAL_MODULE_PATH := $(TARGET_OUT_ETC)/permissions

LOCAL_SRC_FILES := $(LOCAL_MODULE)

include $(BUILD_PREBUILT)
