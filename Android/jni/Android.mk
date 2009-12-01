LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE := libmbfly-jni
LOCAL_SRC_FILES := mbfly.cpp
LOCAL_SHARED_LIBRARIES := libcorecg libsgl libnativehelper libcutils
LOCAL_STATIC_LIBRARIES := libmbfly
LOCAL_C_INCLUDES += $(TARGET_OUT_HEADERS)/libmbfly \
	$(call include-path-for,corecg) \
	$(call include-path-for,libnativehelper)/nativehelper

LOCAL_PRELINK_MODULE := false

include $(BUILD_SHARED_LIBRARY)
