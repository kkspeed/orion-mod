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
		willuslib/math.c \
		willuslib/willusversion.c \
		willuslib/pdfwrite.c \
		willuslib/mem.c \
		willuslib/fontdata.c \
		willuslib/wgs.c \
		willuslib/wzfile.c \
		willuslib/linux.c \
		willuslib/array.c \
		willuslib/token.c \
		willuslib/sys.c \
		willuslib/string.c \
		willuslib/filelist.c \
		willuslib/fontrender.c \
		willuslib/bmpmupdf.c \
		willuslib/point2d.c \
		willuslib/render.c \
		willuslib/gslpolyfit.c \
		willuslib/bmp.c  \
		willuslib/wfile.c  \
		willuslib/ansi.c  \
		mupdf.c \
		list.c  \
		orion_bitmap.c
LOCAL_C_INCLUDES += willuslib/ \
					include_mod/ \
				$(MUPDF_ROOT)/draw \
				$(MUPDF_ROOT)/fitz \
				$(MUPDF_ROOT)/pdf
LOCAL_LIB := $(LOAL_PATH)

LOCAL_LDLIBS := -lz -ljnigraphics -llog -lm 
LOCAL_LIBS += -L$(LOCAL_LIB) -llept -ltess
LOCAL_STATIC_LIBRARIES := mupdfcore mupdfthirdparty
#LOCAL_SHARED_LIBRARIES := lept tess
include $(BUILD_SHARED_LIBRARY)
