#include <jni.h>
#include <time.h>
#include <android/log.h>

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "fitz.h"
#include "mupdf.h"
#include <unistd.h>
#include "list.h"

// MODIFIED
#include "k2pdfopt.h"

#define LOG_TAG "libmupdf"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO,LOG_TAG,__VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR,LOG_TAG,__VA_ARGS__)

/* Set to 1 to enable debug log traces. */
#define DEBUG 0

/* Enable to log rendering times (render each frame 100 times and time) */
#undef TIME_DISPLAY_LIST

#define MAX_SEARCH_HITS (500)




/* Globals */
fz_colorspace *colorspace;
fz_document *doc;
int resolution = 160;
float pageWidth = 100;
float pageHeight = 100;
fz_display_list *currentPageList;
fz_rect currentMediabox;
fz_context *ctx;
int currentPageNumber = -1;
fz_page *currentPage = NULL;
fz_bbox *hit_bbox = NULL;

int rf_width, rf_height;        /* Reflowed page width, height */
int rf_enabled = 0;             /* Is reflow enabled */
int rf_lastPage = -1;
KOPTContext rf_context;

JNIEXPORT void
JNICALL Java_com_artifex_mupdf_MuPDFCore_setReflow(JNIEnv * env, jobject thiz,
                                                   jint reflow)
{
    rf_enabled = reflow;
    rf_lastPage = -1;
}

JNIEXPORT jintArray JNICALL
Java_com_artifex_mupdf_MuPDFCore_setReflowParameters(JNIEnv *env,
                                                     jobject thiz,
                                                     float zoom,
                                                     int dpi,
                                                     int columns,
                                                     int bb_width,
                                                     int bb_height,
                                                     int m_top,
                                                     int m_bottom,
                                                     int m_left,
                                                     int m_right,
                                                     int default_trim,
                                                     int wrap_text,
                                                     int indent,
                                                     int rotation,
                                                     float margin,
                                                     float word_space,
                                                     float quality)
{
    double m_l = m_left / 100.0 * pageWidth;
    double m_r = m_right / 100.0 * pageWidth;
    double m_t = m_top / 100.0 * pageHeight;
    double m_b = m_bottom / 100.0 * pageHeight;

    rf_context.trim = 0;
    if (default_trim) {
        rf_context.trim = 1;        /* trim 0.25 inch? */
        m_l = 0;
        m_r = 0;
        m_t = 0;
        m_b = 0;
    }

    rf_context.wrap = wrap_text;        /* Wrap text */
    rf_context.indent = indent;      /* Indent? */
    rf_context.rotate = rotation;      /* Rotation */
    rf_context.columns = columns; /* Max columns */

    rf_context.offset_x = 0;    /* Not seem to be used */
    rf_context.offset_y = 0;    /* Not seem to be used */

    rf_context.dev_width = bb_width;
    rf_context.dev_height = bb_height;

    rf_context.straighten = 0;  /* Straighten */
    rf_context.justification = -1; /* Default justification */

    rf_context.zoom = zoom;
    rf_context.margin = margin;   /* This should be made an option! */
    rf_context.quality = quality;
    rf_context.contrast = -1;
    rf_context.defect_size = 1.0;
    rf_context.line_spacing = -1.2;
    rf_context.word_spacing = word_space;

    rf_context.bbox.x0 = (m_l > 0.1) ? m_l : 0;
    rf_context.bbox.y0 = (m_t > 0.1) ? m_t : 0;
    rf_context.bbox.x1 = (m_r > 0.1) ? (pageWidth - m_r - 1) : (pageWidth - 1);
    rf_context.bbox.y1 = (m_b > 0.1) ? (pageHeight - m_b - 1) : (pageHeight - 1);

    LOGE("========> zoom: %f, dpi: %d, columns: %d, width: %d, height: %d"
         "top: %f, bottom: %f, left: %f, right: %f, trim: %d, wrap: %d, indent: %d, rot: %d,"
         "margin: %f, word_space: %f, quality: %f",
         zoom, dpi, columns, bb_width, bb_height, m_t, m_b, m_l, m_r, default_trim,
         wrap_text, indent, rotation, margin, word_space, quality);

    /* k2pdfopt_set_params_lite(zoom, dpi, columns, bb_width, bb_height, */
    /*                          m_t, m_b, m_l, m_r); */
}

JNIEXPORT int JNICALL
Java_com_artifex_mupdf_MuPDFCore_openFile(JNIEnv * env, jobject thiz, jstring jfilename, jobject docInfo)
{
	const char *filename;

	int pages = 0;
	int result = 0;

    rf_enabled = 0;
    rf_lastPage = -1;

	filename = (*env)->GetStringUTFChars(env, jfilename, NULL);
	if (filename == NULL)
	{
		LOGE("Failed to get filename");
		return 0;
	}

	/* 128 MB store for low memory devices. Tweak as necessary. */
	ctx = fz_new_context(NULL, NULL, 48<<20);
	if (!ctx)
	{
		LOGE("Failed to initialise context");
		return 0;
	}

	doc = NULL;
	fz_try(ctx)
	{
		colorspace = fz_device_bgr;

		LOGI("Opening document...");
		fz_try(ctx)
		{
			doc = fz_open_document(ctx, (char *)filename);
		}
		fz_catch(ctx)
		{
			fz_throw(ctx, "Cannot open document: '%s'\n", filename);
		}

		char * title = NULL;
		/*
		if (doc->trailer) {
			fz_obj *info = fz_dict_gets(xref->trailer, "Info");
			if (info)
			{
				fz_obj * obj = fz_dict_gets(info, "Title");
				if (obj) {
					title = pdf_to_utf8(obj);
					LOGI("Title  '%s'",title);
				}
			}
		}*/
		pages  = fz_count_pages(doc);
		(*env)->ReleaseStringUTFChars(env, jfilename, filename);

		jclass cls = (*env)->GetObjectClass(env, docInfo);
		jfieldID pageCountF = (*env)->GetFieldID(env, cls, "pageCount", "I");
		jfieldID titleF = (*env)->GetFieldID(env, cls, "title", "Ljava/lang/String;");
		(*env)->SetIntField(env, docInfo, pageCountF, pages);
		if (title) {
			(*env)->SetObjectField(env, docInfo, titleF, ((*env)->NewStringUTF(env, title)));
		}
		LOGI("Document with %i pages opened!", pages);
		result = 1;
	}
	fz_catch(ctx)
	{
		LOGE("Failed: %s", ctx->error->message);
		fz_close_document(doc);
		doc = NULL;
		fz_free_context(ctx);
		ctx = NULL;
	}

	return result;
}

JNIEXPORT int JNICALL
Java_com_artifex_mupdf_MuPDFCore_countPagesInternal(JNIEnv *env, jobject thiz)
{
	return fz_count_pages(doc);
}



JNIEXPORT void JNICALL
Java_com_artifex_mupdf_MuPDFCore_gotoPageInternal(JNIEnv *env, jobject thiz, int page)
{
	fz_bbox bbox;
	fz_device *dev = NULL;

	fz_var(dev);

	if (currentPage != NULL && page != currentPageNumber)
	{
		fz_free_page(doc, currentPage);
		currentPage = NULL;
	}

	/* In the event of an error, ensure we give a non-empty page */
	pageWidth = 100;
	pageHeight = 100;

	currentPageNumber = page;
	LOGE("Goto page %d...", page);
	fz_try(ctx)
	{
		if (currentPageList != NULL)
		{
			fz_free_display_list(ctx, currentPageList);
			currentPageList = NULL;
		}

		currentPage = fz_load_page(doc, page);

        /* When Reflow enabled, reflow the page and get different size */
        if (rf_enabled && (rf_lastPage != page)) {
            k2pdfopt_mupdf_reflow(&rf_context, doc, currentPage, ctx);
            rf_width = rf_context.page_width;
            rf_height = rf_context.page_height;
//            k2pdfopt_rfbmp_size(&rf_width, &rf_height);
            rf_lastPage = page;
        }

		//zoom = resolution / 72;
		currentMediabox = fz_bound_page(doc, currentPage);
		/*ctm = fz_scale(zoom, zoom);
		bbox = fz_round_rect(fz_transform_rect(ctm, currentMediabox));*/
		bbox = fz_round_rect(currentMediabox);
		pageWidth = bbox.x1-bbox.x0;
		pageHeight = bbox.y1-bbox.y0;
	}
	fz_catch(ctx)
	{
		currentPageNumber = page;
		LOGE("cannot make displaylist from page %d", page);
	}
	fz_free_device(dev);
	dev = NULL;
}


JNIEXPORT void JNICALL
Java_com_artifex_mupdf_MuPDFCore_getPageInfo(JNIEnv *env, jobject thiz, int page, jobject info)
{
	fz_bbox bbox;
	fz_device *dev = NULL;

	fz_var(dev);

	/* In the event of an error, ensure we give a non-empty page */
	int pageWidth = 100;
	int pageHeight = 100;
	fz_page * currentPage = NULL;

	LOGI("Get info for page %d...", page);
	fz_try(ctx)
	{
		currentPage = fz_load_page(doc, page);
		fz_rect currentMediabox = fz_bound_page(doc, currentPage);
		/*zoom = resolution / 72;
		ctm = fz_scale(zoom, zoom);
		bbox = fz_round_rect(fz_transform_rect(ctm, currentMediabox));*/
		bbox = fz_round_rect(currentMediabox);
		pageWidth = bbox.x1-bbox.x0;
		pageHeight = bbox.y1-bbox.y0;
	}
	fz_catch(ctx)
	{
		LOGE("Error on processing page info %d", page);
	}

	if (currentPage != NULL) {
		fz_free_page(doc, currentPage);
		currentPage = NULL;
	}
	fz_free_device(dev);
	dev = NULL;

    /* If reflow is enabled, trick the width/height report
     * so that the page will be split into patches
     */
    if (rf_enabled) {
        pageWidth = rf_width;
        pageHeight = rf_height;
    }

	jclass cls = (*env)->GetObjectClass(env, info);
	jfieldID width = (*env)->GetFieldID(env, cls, "width", "I");
	jfieldID height = (*env)->GetFieldID(env, cls, "height", "I");

	 (*env)->SetIntField(env, info, width, pageWidth);
	 (*env)->SetIntField(env, info, height, pageHeight);

	//end = clock();

	LOGI("Page %d info: %dx%d", page, pageWidth, pageHeight);
}

JNIEXPORT float JNICALL
Java_com_artifex_mupdf_MuPDFCore_getPageWidth(JNIEnv *env, jobject thiz)
{
	LOGE("PageWidth=%g", pageWidth);
	return pageWidth;
}

JNIEXPORT float JNICALL
Java_com_artifex_mupdf_MuPDFCore_getPageHeight(JNIEnv *env, jobject thiz)
{
	LOGE("PageHeight=%g", pageHeight);
	return pageHeight;
}

/* Routine to draw the reflow page */
void drawReflowedPage(unsigned char* buf, int pageW, int pageH,
                      int patchX, int patchY, int patchW, int patchH)
{
    uint8_t *pmptr = rf_context.data;

    pmptr += (patchY * patchW) + patchX;

    int i, k;
    int x, y;
    uint8_t color = 0;
    int bmp_size = rf_width * rf_height;

    for (y=0; y<pageH; y++) {
        for (x=0; x<pageW * 4; x+=4) {
            i = y * pageW * 4 + x;
            k = i >> 2;

            if (k >= bmp_size)
                color = 255;
            else
                color = pmptr[k];

            buf[i] = color;
            buf[i+1] = color;
            buf[i+2] = color;
            buf[i+3] = color;
        }
    }
}

JNIEXPORT jintArray JNICALL
Java_com_artifex_mupdf_MuPDFCore_drawPage(JNIEnv *env, jobject thiz, float zoom,
		int pageW, int pageH, int patchX, int patchY, int patchW, int patchH)
{
	LOGI("==================Start Rendering==============");
    fz_device *dev = NULL;
	fz_matrix ctm;
	fz_bbox bbox;
	fz_pixmap *pix = NULL;
	float xscale, yscale;
	fz_bbox rect;

	fz_var(pix);
	fz_var(dev);

	int num_pixels = pageW * pageH;
	jintArray jints; /* return value */
	int *jbuf;
	clock_t start, end;
	double elapsed;

	/* Call mupdf to render display list to screen */
	LOGI("Rendering page=%dx%d patch=[%d,%d,%d,%d], rotation=%d",
			pageW, pageH, patchX, patchY, patchW, patchH, 0);

	jints = (*env)->NewIntArray(env, num_pixels);
	jbuf = (*env)->GetIntArrayElements(env, jints, NULL);
	start = clock();

    if (rf_enabled) {
        drawReflowedPage((unsigned char*)jbuf, pageW, pageH, patchX, patchY,
                         patchW, patchH);
    } else {
        rect.x0 = patchX;
        rect.y0 = patchY;
        rect.x1 = patchX + patchW;
        rect.y1 = patchY + patchH;

        fz_try(ctx)
        {
            if (currentPageList == NULL)
            {
                /* Render to list */
                currentPageList = fz_new_display_list(ctx);
                dev = fz_new_list_device(ctx, currentPageList);

                fz_run_page(doc, currentPage, dev, fz_identity, NULL);

                fz_free_device(dev);
            }
            pix = fz_new_pixmap_with_bbox_and_data(ctx, colorspace, rect, (unsigned char *)jbuf);
            if (currentPageList == NULL)
            {
                fz_clear_pixmap_with_value(ctx, pix, 0xd0);
                break;
            }
            fz_clear_pixmap_with_value(ctx, pix, 0xff);

            LOGE("zoom = %f", zoom);
            ctm = fz_scale(zoom, zoom);
            bbox = fz_round_rect(fz_transform_rect(ctm,currentMediabox));
            LOGI("x=[%d,%d,%d,%d], %dx%d",
                 bbox.x0, bbox.x1, bbox.y0, bbox.y1, bbox.x1-bbox.x0, bbox.y1-bbox.y0, 0);
            dev = fz_new_draw_device(ctx, pix);
            fz_run_display_list(currentPageList, dev, ctm, rect, NULL);
            fz_free_device(dev);
            dev = NULL;
            fz_drop_pixmap(ctx, pix);
        }
        fz_catch(ctx)
        {
            fz_free_device(dev);
            LOGE("Render failed");
        }
    }

    orion_updateContrast((unsigned char *) jbuf, num_pixels*4);
    (*env)->ReleaseIntArrayElements(env, jints, jbuf, 0);

    LOGE("Rendered");
	end = clock();
	elapsed = ((double) (end - start)) / CLOCKS_PER_SEC;
	LOGI("Total murendering time %lf", elapsed);

	return jints;
}

static fz_text_char textcharat(fz_text_page *page, int idx)
{
	static fz_text_char emptychar = { {0,0,0,0}, ' ' };
	fz_text_block *block;
	fz_text_line *line;
	fz_text_span *span;
	int ofs = 0;
	for (block = page->blocks; block < page->blocks + page->len; block++)
	{
		for (line = block->lines; line < block->lines + block->len; line++)
		{
			for (span = line->spans; span < line->spans + line->len; span++)
			{
				if (idx < ofs + span->len)
					return span->text[idx - ofs];
				/* pseudo-newline */
				if (span + 1 == line->spans + line->len)
				{
					if (idx == ofs + span->len)
						return emptychar;
					ofs++;
				}
				ofs += span->len;
			}
		}
	}
	return emptychar;
}

static int
charat(fz_text_page *page, int idx)
{
	return textcharat(page, idx).c;
}

static fz_bbox
bboxcharat(fz_text_page *page, int idx)
{
	return fz_round_rect(textcharat(page, idx).bbox);
}

static int
textlen(fz_text_page *page)
{
	fz_text_block *block;
	fz_text_line *line;
	fz_text_span *span;
	int len = 0;
	for (block = page->blocks; block < page->blocks + page->len; block++)
	{
		for (line = block->lines; line < block->lines + block->len; line++)
		{
			for (span = line->spans; span < line->spans + line->len; span++)
				len += span->len;
			len++; /* pseudo-newline */
		}
	}
	return len;
}

static int
match(fz_text_page *page, const char *s, int n)
{
	int orig = n;
	int c;
	while (*s) {
		s += fz_chartorune(&c, (char *)s);
		if (c == ' ' && charat(page, n) == ' ') {
			while (charat(page, n) == ' ')
				n++;
		} else {
			if (tolower(c) != tolower(charat(page, n)))
				return 0;
			n++;
		}
	}
	return n - orig;
}

static int
countOutlineItems(fz_outline *outline)
{
	int count = 0;

	while (outline)
	{
		if (outline->dest.kind == FZ_LINK_GOTO
				&& outline->dest.ld.gotor.page >= 0
				&& outline->title)
			count++;

		count += countOutlineItems(outline->down);
		outline = outline->next;
	}

	return count;
}

static int
fillInOutlineItems(JNIEnv * env, jclass olClass, jmethodID ctor, jobjectArray arr, int pos, fz_outline *outline, int level)
{
	while (outline)
	{
		if (outline->dest.kind == FZ_LINK_GOTO)
		{
			int page = outline->dest.ld.gotor.page;
			if (page >= 0 && outline->title)
			{
				jobject ol;
				jstring title = (*env)->NewStringUTF(env, outline->title);
				if (title == NULL) return -1;
				ol = (*env)->NewObject(env, olClass, ctor, level, title, page);
				if (ol == NULL) return -1;
				(*env)->SetObjectArrayElement(env, arr, pos, ol);
				(*env)->DeleteLocalRef(env, ol);
				(*env)->DeleteLocalRef(env, title);
				pos++;
			}
		}
		pos = fillInOutlineItems(env, olClass, ctor, arr, pos, outline->down, level+1);
		if (pos < 0) return -1;
		outline = outline->next;
	}

	return pos;
}

JNIEXPORT jboolean JNICALL
Java_com_artifex_mupdf_MuPDFCore_needsPasswordInternal(JNIEnv * env, jobject thiz)
{
	return fz_needs_password(doc) ? JNI_TRUE : JNI_FALSE;
}

JNIEXPORT jboolean JNICALL
Java_com_artifex_mupdf_MuPDFCore_authenticatePasswordInternal(JNIEnv *env, jobject thiz, jstring password)
{
	const char *pw;
	int         result;
	pw = (*env)->GetStringUTFChars(env, password, NULL);
	if (pw == NULL)
		return JNI_FALSE;

	result = fz_authenticate_password(doc, (char *)pw);
	(*env)->ReleaseStringUTFChars(env, password, pw);
	return result;
}

JNIEXPORT jboolean JNICALL
Java_com_artifex_mupdf_MuPDFCore_hasOutlineInternal(JNIEnv * env, jobject thiz)
{
	fz_outline *outline = fz_load_outline(doc);
	return (outline == NULL) ? JNI_FALSE : JNI_TRUE;
}

JNIEXPORT jobjectArray JNICALL
Java_com_artifex_mupdf_MuPDFCore_getOutlineInternal(JNIEnv * env, jobject thiz)
{
	jclass        olClass;
	jmethodID     ctor;
	jobjectArray  arr;
	jobject       ol;
	fz_outline   *outline;
	int           nItems;

	olClass = (*env)->FindClass(env, "com/artifex/mupdf/OutlineItem");
	if (olClass == NULL) return NULL;
	ctor = (*env)->GetMethodID(env, olClass, "<init>", "(ILjava/lang/String;I)V");
	if (ctor == NULL) return NULL;

	outline = fz_load_outline(doc);
	nItems = countOutlineItems(outline);

	arr = (*env)->NewObjectArray(env,
					nItems,
					olClass,
					NULL);
	if (arr == NULL) return NULL;

	return fillInOutlineItems(env, olClass, ctor, arr, 0, outline, 0) > 0
			? arr
			:NULL;
}

JNIEXPORT jobjectArray JNICALL
Java_com_artifex_mupdf_MuPDFCore_searchPage(JNIEnv * env, jobject thiz, jstring jtext)
{
	jclass         rectClass;
	jmethodID      ctor;
	jobjectArray   arr;
	jobject        rect;
	fz_text_sheet *sheet = NULL;
	fz_text_page  *text = NULL;
	fz_device     *dev  = NULL;
	float          zoom;
	fz_matrix      ctm;
	int            pos;
	int            len;
	int            i, n;
	int            hit_count = 0;
	const char    *str;

	rectClass = (*env)->FindClass(env, "android/graphics/RectF");
	if (rectClass == NULL) return NULL;
	ctor = (*env)->GetMethodID(env, rectClass, "<init>", "(FFFF)V");
	if (ctor == NULL) return NULL;
	str = (*env)->GetStringUTFChars(env, jtext, NULL);
	if (str == NULL) return NULL;

	fz_var(sheet);
	fz_var(text);
	fz_var(dev);

	fz_try(ctx)
	{
		fz_rect rect;

		if (hit_bbox == NULL)
			hit_bbox = fz_malloc_array(ctx, MAX_SEARCH_HITS, sizeof(*hit_bbox));

		zoom = resolution / 72;
		ctm = fz_scale(zoom, zoom);
		rect = fz_transform_rect(ctm, currentMediabox);
		sheet = fz_new_text_sheet(ctx);
		text = fz_new_text_page(ctx, rect);
		dev  = fz_new_text_device(ctx, sheet, text);
		fz_run_page(doc, currentPage, dev, ctm, NULL);
		fz_free_device(dev);
		dev = NULL;

		len = textlen(text);
		for (pos = 0; pos < len; pos++)
		{
			fz_bbox rr = fz_empty_bbox;
			n = match(text, str, pos);
			for (i = 0; i < n; i++)
				rr = fz_union_bbox(rr, bboxcharat(text, pos + i));

			if (!fz_is_empty_bbox(rr) && hit_count < MAX_SEARCH_HITS)
				hit_bbox[hit_count++] = rr;
		}
	}
	fz_always(ctx)
	{
		fz_free_text_page(ctx, text);
		fz_free_text_sheet(ctx, sheet);
		fz_free_device(dev);
	}
	fz_catch(ctx)
	{
		jclass cls;
		(*env)->ReleaseStringUTFChars(env, jtext, str);
		cls = (*env)->FindClass(env, "java/lang/OutOfMemoryError");
		if (cls != NULL)
			(*env)->ThrowNew(env, cls, "Out of memory in MuPDFCore_searchPage");
		(*env)->DeleteLocalRef(env, cls);

		return NULL;
	}

	(*env)->ReleaseStringUTFChars(env, jtext, str);

	arr = (*env)->NewObjectArray(env,
					hit_count,
					rectClass,
					NULL);
	if (arr == NULL) return NULL;

	for (i = 0; i < hit_count; i++) {
		rect = (*env)->NewObject(env, rectClass, ctor,
				(float) (hit_bbox[i].x0),
				(float) (hit_bbox[i].y0),
				(float) (hit_bbox[i].x1),
				(float) (hit_bbox[i].y1));
		if (rect == NULL)
			return NULL;
		(*env)->SetObjectArrayElement(env, arr, i, rect);
		(*env)->DeleteLocalRef(env, rect);
	}

	return arr;
}

JNIEXPORT void JNICALL
Java_com_artifex_mupdf_MuPDFCore_destroying(JNIEnv * env, jobject thiz)
{
	fz_free(ctx, hit_bbox);
	hit_bbox = NULL;
	fz_free_display_list(ctx, currentPageList);
	currentPageList = NULL;
	if (currentPage != NULL)
	{
		fz_free_page(doc, currentPage);
		currentPage = NULL;
	}
	fz_close_document(doc);
	doc = NULL;
}

JNIEXPORT jobjectArray JNICALL
Java_com_artifex_mupdf_MuPDFCore_getPageLinksInternal(JNIEnv * env, jobject thiz, int pageNumber)
{
	jclass       linkInfoClass;
	jmethodID    ctor;
	jobjectArray arr;
	jobject      linkInfo;
	fz_matrix    ctm;
	float        zoom;
	fz_link     *list;
	fz_link     *link;
	int          count;

	linkInfoClass = (*env)->FindClass(env, "com/artifex/mupdf/LinkInfo");
	if (linkInfoClass == NULL) return NULL;
	ctor = (*env)->GetMethodID(env, linkInfoClass, "<init>", "(FFFFI)V");
	if (ctor == NULL) return NULL;

	Java_com_artifex_mupdf_MuPDFCore_gotoPageInternal(env, thiz, pageNumber);
	if (currentPageNumber == -1 || currentPage == NULL)
		return NULL;

	zoom = resolution / 72;
	ctm = fz_scale(zoom, zoom);

	list = fz_load_links(doc, currentPage);
	count = 0;
	for (link = list; link; link = link->next)
	{
		if (link->dest.kind == FZ_LINK_GOTO)
			count++ ;
	}

	arr = (*env)->NewObjectArray(env, count, linkInfoClass, NULL);
	if (arr == NULL) return NULL;

	count = 0;
	for (link = list; link; link = link->next)
	{
		if (link->dest.kind == FZ_LINK_GOTO)
		{
			fz_rect rect = fz_transform_rect(ctm, link->rect);

			linkInfo = (*env)->NewObject(env, linkInfoClass, ctor,
					(float)rect.x0, (float)rect.y0, (float)rect.x1, (float)rect.y1,
					link->dest.ld.gotor.page);
			if (linkInfo == NULL) return NULL;
			(*env)->SetObjectArrayElement(env, arr, count, linkInfo);
			(*env)->DeleteLocalRef(env, linkInfo);

			count ++;
		}
	}

	return arr;
}

JNIEXPORT int JNICALL
Java_com_artifex_mupdf_MuPDFCore_getPageLink(JNIEnv * env, jobject thiz, int pageNumber, float x, float y)
{
	fz_matrix ctm;
	float zoom;
	fz_link *link;
	fz_point p;

	Java_com_artifex_mupdf_MuPDFCore_gotoPageInternal(env, thiz, pageNumber);
	if (currentPageNumber == -1 || currentPage == NULL)
		return -1;

	p.x = x;
	p.y = y;

	/* Ultimately we should probably return a pointer to a java structure
	 * with the link details in, but for now, page number will suffice.
	 */
	zoom = resolution / 72;
	ctm = fz_scale(zoom, zoom);
	ctm = fz_invert_matrix(ctm);

	p = fz_transform_point(ctm, p);

	for (link = fz_load_links(doc, currentPage); link; link = link->next)
	{
		if (p.x >= link->rect.x0 && p.x <= link->rect.x1)
			if (p.y >= link->rect.y0 && p.y <= link->rect.y1)
				break;
	}

	if (link == NULL)
		return -1;

	if (link->dest.kind == FZ_LINK_URI)
	{
		//gotouri(link->dest.ld.uri.uri);
		return -1;
	}
	else if (link->dest.kind == FZ_LINK_GOTO)
		return link->dest.ld.gotor.page;
	return -1;
}


JNIEXPORT jstring JNICALL
Java_com_artifex_mupdf_MuPDFCore_getText(JNIEnv *env,
                                         jobject thiz,
                                         int pageNumber,
                                         int startX,
                                         int startY,
                                         int width,
                                         int height)
{
	LOGI("==================Start Text Extraction==============");
	jstring * result;

    if (rf_enabled) {
        char text[256];

        LOGI("startX: %d, startY: %d, width: %d, height: %d", startX, startY,
             width, height);

        int status = ocrtess_init("/sdcard/data/tesseract/", "eng", 1, stdout);

        LOGI("======================= Initialized: %d", status);

        ocrtess_single_word_from_bmp8(text,
                                      255,
                                      rf_context.bmp,
                                      startX,
                                      startY,
                                      startX + width,
                                      startY + height,
                                      1,
                                      0,
                                      1,
                                      NULL);
        LOGI("HERE");

		result = (*env)->NewStringUTF(env, text);
        LOGI("OCR WORDS: %s\n", text);
        ocrtess_end();
        return result;
    }

	fz_display_list *pageList = NULL;
	fz_page *currentPagex = NULL;

	fz_device *dev = NULL;
	fz_matrix ctm;
	fz_rect rect;

	fz_var(dev);
	clock_t start, end;
	double elapsed;



	LOGI("Start text extraction: rectangle=[%d,%d,%d,%d]", startX, startY, width, height);

	rect.x0 = startX;
	rect.y0 = startY;
	rect.x1 = startX + width;
	rect.y1 = startY + height;

	start = clock();

	Arraylist values = arraylist_create();

	fz_try(ctx)
	{
		fz_text_sheet * sheet = fz_new_text_sheet(ctx);
		fz_text_page * page = fz_new_text_page(ctx, rect);
		dev = fz_new_text_device(ctx, sheet, page);

		currentPagex = fz_load_page(doc, pageNumber);
        fz_run_page(doc, currentPagex, dev, fz_identity, NULL);

        fz_text_block *block;
        fz_text_line *line;
        fz_text_span *span;
        fz_text_char *ch;
        char utf[10];
        int i, n;

        for (block = page->blocks; block < page->blocks + page->len; block++)
        {
            for (line = block->lines; line < block->lines + block->len; line++)
            {
                for (span = line->spans; span < line->spans + line->len; span++)
                {
                    for (ch = span->text; ch < span->text + span->len; ch++)
                    {
                        fz_rect span_rect = ch->bbox;
                        fz_rect intr = fz_intersect_rect(span_rect, rect);
                        if (!fz_is_empty_rect(intr) && ((intr.x1-intr.x0)*(intr.y1-intr.y0) / ((span_rect.x1-span_rect.x0)*(span_rect.y1-span_rect.y0)) > 0.4)) {
                            n = fz_runetochar(utf, ch->c);
                            for (i = 0; i < n; i++) {
                                arraylist_add(values, utf[i]);
                            }
                        }
                    }
                }
            }
        }

        arraylist_add(values, 0);

		LOGI("Data: %s", arraylist_getData(values));
		result = (*env)->NewStringUTF(env, arraylist_getData(values));
		arraylist_free(values);

		fz_free_device(dev);
		fz_free_text_sheet(ctx, sheet);
		fz_free_text_page(ctx, page);

		dev = NULL;
	}
	fz_catch(ctx)
	{
		fz_free_device(dev);
		LOGE("Render failed");
	}
	end = clock();
	elapsed = ((double) (end - start)) / CLOCKS_PER_SEC;

	fz_free_display_list(ctx, pageList);

    fz_free_page(doc, currentPagex);

	LOGI("Total text etraction time %lf", elapsed);
	return result;
}
