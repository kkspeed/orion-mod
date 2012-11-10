LOCAL_PATH := $(call my-dir)
TOP_LOCAL_PATH := $(LOCAL_PATH)

MUPDF_ROOT := ..

include $(TOP_LOCAL_PATH)/Core.mk
include $(TOP_LOCAL_PATH)/ThirdParty.mk

include $(CLEAR_VARS)
LOCAL_MODULE    := mupdf
LOCAL_SRC_FILES := k2pdfopt.c \
		mupdf.c \
		list.c  \
		orion_bitmap.c
LOCAL_C_INCLUDES += $(MUPDF_ROOT)/draw \
				$(MUPDF_ROOT)/fitz \
				$(MUPDF_ROOT)/pdf
LOCAL_LIB := $(LOAL_PATH)

LOCAL_LDLIBS := -lz -ljnigraphics -llog -lm 
LOCAL_STATIC_LIBRARIES := mupdfcore mupdfthirdparty
include $(BUILD_SHARED_LIBRARY)
