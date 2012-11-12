LOCAL_PATH := $(call my-dir)
TOP_LOCAL_PATH := $(LOCAL_PATH)

MUPDF_ROOT := ..

include $(TOP_LOCAL_PATH)/Core.mk
include $(TOP_LOCAL_PATH)/ThirdParty.mk

include $(CLEAR_VARS)
LOCAL_MODULE := lept
LOCAL_SRC_FILES := liblept.so
include $(PREBUILT_SHARED_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := tess
LOCAL_SRC_FILES := libtess.so
include $(PREBUILT_SHARED_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE    := mupdf
LOCAL_SRC_FILES := k2pdfopt.c \
		mupdf.c \
		list.c  \
		ocr.c\
		ocrtess.c \
		orion_bitmap.c
LOCAL_C_INCLUDES += $(MUPDF_ROOT)/draw \
					$(MUPDF_ROOT)/fitz \
					$(MUPDF_ROOT)/pdf
LOCAL_LIB := $(LOCAL_PATH)

LOCAL_LDLIBS := -lz -ljnigraphics -llog -lm 
LOCAL_LIBS += -llept -ltess
LOCAL_STATIC_LIBRARIES := mupdfcore mupdfthirdparty
LOCAL_SHARED_LIBRARIES := lept tess
include $(BUILD_SHARED_LIBRARY)
