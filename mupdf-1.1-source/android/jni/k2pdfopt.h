/*
 ** k2pdfopt.h   K2pdfopt optimizes PDF/DJVU files for mobile e-readers
 **              (e.g. the Kindle) and smartphones. It works well on
 **              multi-column PDF/DJVU files. K2pdfopt is freeware.
 **
 ** Copyright (C) 2012  http://willus.com
 **
 ** This program is free software: you can redistribute it and/or modify
 ** it under the terms of the GNU Affero General Public License as
 ** published by the Free Software Foundation, either version 3 of the
 ** License, or (at your option) any later version.
 **
 ** This program is distributed in the hope that it will be useful,
 ** but WITHOUT ANY WARRANTY; without even the implied warranty of
 ** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 ** GNU Affero General Public License for more details.
 **
 ** You should have received a copy of the GNU Affero General Public License
 ** along with this program.  If not, see <http://www.gnu.org/licenses/>.
 **
 */

#ifndef _K2PDFOPT_H
#define _K2PDFOPT_H

#include <fitz-internal.h>
//#include <libdjvu/ddjvuapi.h>

//typedef unsigned char  uint8_t;

typedef struct {
	int red[256];
	int green[256];
	int blue[256];
	unsigned char *data; /* Top to bottom in native type, bottom to */
	/* top in Win32 type.                      */
	int width; /* Width of image in pixels */
	int height; /* Height of image in pixels */
	int bpp; /* Bits per pixel (only 8 or 24 allowed) */
	int size_allocated;
	int type; /* See defines above for WILLUSBITMAP_TYPE_... */
} WILLUSBITMAP;

typedef struct
{
    int r,c;   /* row,column position of left baseline of the word, e.g. the bottom left of */
    /* most capital letters */
    int w,h;   /* width and height of word in pixels */
    double maxheight;  /* max height of any letter from baseline in pixels */
    double lcheight;  /* height of a lowercase letter in pixels */
    int rot;   /* rotation angle of word in degrees */
    char *text;  /* ASCII text of word */
} OCRWORD;

typedef struct
{
    OCRWORD *word;
    int n,na;
} OCRWORDS;


typedef struct KOPTContext {
    int trim;
    int wrap;
    int indent;
    int rotate;
    int columns;
    int offset_x;
    int offset_y;
    int dev_width;
    int dev_height;
    int page_width;
    int page_height;
    int straighten;
    int justification;

    double zoom;
    double margin;
    double quality;
    double contrast;
    double defect_size;
    double line_spacing;
    double word_spacing;

    WILLUSBITMAP *bmp;
    uint8_t *data;
    fz_rect bbox;

    int ocr_lang;
} KOPTContext;



void k2pdfopt_mupdf_reflow(KOPTContext *kc, fz_document *doc, fz_page *page, fz_context *ctx);
/* void k2pdfopt_djvu_reflow(KOPTContext *kc, ddjvu_page_t *page, ddjvu_context_t *ctx, ddjvu_render_mode_t mode, ddjvu_format_t *fmt); */

void ocrtess_single_word_from_bmp8(char *text,int maxlen,WILLUSBITMAP *bmp8,
        int x1,int y1,int x2,int y2,
        int ocr_type,int allow_spaces,
        int std_proc,FILE *out);
unsigned char *bmp_rowptr_from_top(WILLUSBITMAP *bmp, int row);

void willus_dmem_alloc_warn(int index, void **ptr, int size,
		char *funcname, int exitcode);
void willus_dmem_free(int index, double **ptr, char *funcname);
int willus_mem_alloc_warn(void **ptr, int size, char *name, int exitcode);
void willus_mem_free(double **ptr, char *name);
int willus_mem_realloc_robust_warn(void **ptr,int newsize,int oldsize,char *name, int exitcode);
void clean_line(char *buf);
int in_string(char *buffer,char *pattern);
int strnicmp(const char *s1,const char *s2,int n);
#endif
