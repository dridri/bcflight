LOCAL_PATH:= $(call my-dir)

# ------------------------------------------------------------------
# Static library
# ------------------------------------------------------------------

include $(CLEAR_VARS)

LOCAL_MODULE := libgammaengine

ifneq (,$(filter $(TARGET_ARCH), x86_64 arm64 arm64-v8 mips64))
	ARCH = 64
else
	ARCH = 32
endif

SRC := $(wildcard ../*.cpp)

$(info $(SRC))

LOCAL_SRC_FILES = $(addprefix ../, $(SRC))

LOCAL_CPPFLAGS := -Os -Wall -Wno-pmf-conversions -Wno-unused -std=gnu++0x -std=c++11 -fPIC -I../ -DGE_ANDROID

LOCAL_EXPORT_C_INCLUDES := $(LOCAL_PATH)

LOCAL_LDLIBS    := -lgammaengine -lmad -lfreetype2-static -ljpeg9 -lpng -lz -lm -llog -landroid -lEGL -lGLESv2 -lOpenSLES
LOCAL_WHOLE_STATIC_LIBRARIES := android_native_app_glue

LOCAL_C_INCLUDES += $(NDK_ROOT)/sources/cxx-stl/gnu-libstdc++/4.9/include
ifneq (,$(filter $(TARGET_ARCH), arm armeabi))
	LOCAL_C_INCLUDES += $(NDK_ROOT)/sources/cxx-stl/gnu-libstdc++/4.9/libs/armeabi/include
	LOCAL_LDLIBS += $(NDK_ROOT)/sources/cxx-stl/gnu-libstdc++/4.9/libs/armeabi/libgnustl_static.a
else ifneq (,$(filter $(TARGET_ARCH), arm64))
	LOCAL_C_INCLUDES += $(NDK_ROOT)/sources/cxx-stl/gnu-libstdc++/4.9/libs/arm64-v8a/include
	LOCAL_LDLIBS += $(NDK_ROOT)/sources/cxx-stl/gnu-libstdc++/4.9/libs/arm64-v8a/libgnustl_static.a
else
	LOCAL_C_INCLUDES += $(NDK_ROOT)/sources/cxx-stl/gnu-libstdc++/4.9/libs/$(TARGET_ARCH)/include
	LOCAL_LDLIBS += $(NDK_ROOT)/sources/cxx-stl/gnu-libstdc++/4.9/libs/$(TARGET_ARCH)/libgnustl_static.a
endif
$(warning $(LOCAL_C_INCLUDES))

# include $(BUILD_STATIC_LIBRARY)
include $(BUILD_SHARED_LIBRARY)

$(call import-module, android/native_app_glue)
