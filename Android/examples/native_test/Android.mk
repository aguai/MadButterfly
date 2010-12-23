LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_MODULE := native_test
LOCAL_STATIC_LIBRARIES := libmbfly
LOCAL_SHARED_LIBRARIES := libsgl
LOCAL_C_INCLUDES += $(call include-path-for,corecg) \
	$(TARGET_OUT_HEADERS)/libmbfly
LOCAL_SRC_FILES := native_test.cpp

include $(BUILD_EXECUTABLE)

