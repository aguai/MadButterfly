LOCAL_PATH:= $(call my-dir)
MB_LOCAL_PATH := $(LOCAL_PATH)
include $(CLEAR_VARS)

LOCAL_MODULE := MadButterfly
LOCAL_CONFIGURE := $(LOCAL_PATH)/../../configure
LOCAL_CONFIGURE_ARGS := --enable-skia --config-cache
LOCAL_CFLAGS := -I$(shell pwd)/$(call include-path-for,corecg) \
	-I$(shell pwd)/$(call include-path-for,corecg)/../effects/ \
	-I$(shell pwd)/$(call include-path-for,frameworks-base)
LOCAL_CONFIGURE_CACHE := config.cache

include $(BUILD_AUTOCONF)

MB_INTERMEDIATES:=$(strip $(intermediates))

$(LOCAL_BUILT_MODULE): $(MB_INTERMEDIATES)/build/config.cache

$(MB_INTERMEDIATES)/build/config.cache:
	$(hide) mkdir -p $(MB_INTERMEDIATES)/build/;
	cp $(strip $(MB_LOCAL_PATH))/config.cache $(MB_INTERMEDIATES)/build/

include $(CLEAR_VARS)

LOCAL_MODULE:= libmbfly
LOCAL_MODULE_SUFFIX:= .a
LOCAL_SRC_FILES:= $(MB_INTERMEDIATES)/build/src/.libs/libmbfly.a
LOCAL_MODULE_CLASS:= STATIC_LIBRARIES
LOCAL_REQUIRED_MODULES:= MadButterfly
LOCAL_COPY_HEADERS:= \
		../../src/include/mb_prop.h \
		../../src/include/mb_graph_engine.h \
		../../src/include/mb_redraw_man.h \
		../../src/include/mb.h \
		../../src/include/mb_graph_engine_cairo.h \
		../../src/include/mb_shapes.h \
		../../src/include/mb_X_supp.h \
		../../src/include/mb_graph_engine_skia.h \
		../../src/include/mb_so.h \
		../../src/include/mb_af.h \
		../../src/include/mb_img_ldr.h \
		../../src/include/mb_timer.h \
		../../src/include/mb_ani_menu.h \
		../../src/include/mb_obj.h \
		../../src/include/mb_tools.h \
		../../src/include/mb_animate.h \
		../../src/include/mb_observer.h \
		../../src/include/mb_types.h \
		../../src/include/mb_basic_types.h \
		../../src/include/mb_paint.h \
		../../src/include/mbbutton.h
LOCAL_COPY_HEADERS_TO:= libmbfly

include $(BUILD_DUMMY)

$(eval $(call copy-one-header,$(MB_INTERMEDIATES)/build/include/mb_config.h,$(TARGET_OUT_HEADERS)/$(LOCAL_COPY_HEADERS_TO)/mb_config.h))
all_copied_headers: $(TARGET_OUT_HEADERS)/$(LOCAL_COPY_HEADERS_TO)/mb_config.h

$(MB_INTERMEDIATES)/build/src/.libs/libmbfly.a: MadButterfly

