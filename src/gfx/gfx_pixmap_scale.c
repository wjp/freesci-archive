/* Required defines:
** FUNCNAME: Function name
** SIZETYPE: Type used for each pixel
** EXTRA_BYTE_OFFSET: Extra source byte offset for copying (used on big-endian machines in 24 bit mode)
*/

#define EXTEND_COLOR(x) (unsigned) ((((unsigned) x) << 24) | (((unsigned) x) << 16) | (((unsigned) x) << 8) | ((unsigned) x))
#define PALETTE_MODE mode->palette

void
FUNCNAME(gfx_mode_t *mode, gfx_pixmap_t *pxm, int scale)
{
	SIZETYPE result_colors[GFX_PIC_COLORS];
	SIZETYPE alpha_color = 0xffffffff & mode->alpha_mask;
	int xfact = (scale)? mode->xfact: 1;
	int yfact = (scale)? mode->yfact: 1;
	int widthc, heightc; /* Width duplication counter */
	int line_width = xfact * pxm->index_xl;
	int bytespp = mode->bytespp;
	int x, y;
	int i;
	byte *src = pxm->index_data;
	byte *dest = pxm->data;
	byte *alpha_dest = pxm->alpha_map;
	int using_alpha = pxm->colors_nr < GFX_PIC_COLORS;
	int separate_alpha_map = (!mode->alpha_mask) && using_alpha;

        assert(bytespp == COPY_BYTES);

	if (separate_alpha_map && !alpha_dest)
		alpha_dest = pxm->alpha_map = malloc(pxm->index_xl * xfact * pxm->index_yl * yfact);

	if (!separate_alpha_map)
		result_colors[GFX_COLOR_INDEX_TRANSPARENT] = alpha_color;

	/* Calculate all colors */
	for (i = 0; i < pxm->colors_nr; i++) {
		int col;

		if (PALETTE_MODE)
			col = pxm->colors[i].global_index;
		else {
			col = mode->red_mask & ((EXTEND_COLOR(pxm->colors[i].r)) >> mode->red_shift);
			col |= mode->green_mask & ((EXTEND_COLOR(pxm->colors[i].g)) >> mode->green_shift);
			col |= mode->blue_mask & ((EXTEND_COLOR(pxm->colors[i].b)) >> mode->blue_shift);
		}
		result_colors[i] = col;
	}

	for (y = 0; y < pxm->index_yl; y++) {
		byte *prev_dest = dest;
		byte *prev_alpha_dest = alpha_dest; 

		for (x = 0; x < pxm->index_xl; x++) {
			int isalpha;
                        byte temp[sizeof(SIZETYPE)*2];
			SIZETYPE col = result_colors[isalpha = *src++] << (EXTRA_BYTE_OFFSET * 8);
			isalpha = (isalpha == 255) && using_alpha;

			/* O(n) loops. There is an O(ln(n)) algorithm for this, but its slower for small n (which we're optimizing for here) */
			for (widthc = 0; widthc < xfact; widthc++) {
				memcpy(dest, &col, COPY_BYTES);
				dest += COPY_BYTES;
			}

			if (separate_alpha_map) { /* Set separate alpha map */
				memset(alpha_dest, (isalpha)? 255 : 0, xfact);
				alpha_dest += xfact;
			}
		}

		/* Copies each line. O(n) iterations; again, this could be optimized to O(ln(n)) for very high resolutions,
		** but that wouldn't really help that much, as the same amount of data still would have to be transferred.
		*/
		for (heightc = 1; heightc < yfact; heightc++) {
			memcpy(dest, prev_dest, line_width * bytespp);
			dest += line_width * bytespp;
			if (separate_alpha_map) {
				memcpy(alpha_dest, prev_alpha_dest, line_width);
				alpha_dest += line_width;
			}
		}
	}
}




/* linear filter: Macros (in reverse order) */

#define X_CALC_INTENSITY_NORMAL (ctexel[i] << 16) + ((linecolor[i])*(256-column_valuator)) + ((othercolumn[i]*column_valuator))*(256-line_valuator)
#define X_CALC_INTENSITY_CENTER (ctexel[i] << 16) + ((linecolor[i])*(256-column_valuator))

#define WRITE_XPART(X_CALC_INTENSITY, DO_X_STEP) \
				for (subx = 0; subx < ((DO_X_STEP)? (xfact >> 1) : 1); subx++) { \
                                        unsigned int intensity; \
					wrcolor = 0; \
					for (i = 0; i < 4; i++) { \
						intensity = X_CALC_INTENSITY; \
						wrcolor |= (intensity >> shifts[i]) & masks[i]; \
					} \
                                        if (separate_alpha_map) \
                                                *alpha_wrpos++ = intensity >> 24; \
					wrcolor <<= (EXTRA_BYTE_OFFSET * 8); \
					memcpy(wrpos, &wrcolor, COPY_BYTES); \
					wrpos += COPY_BYTES; \
					if (DO_X_STEP) \
                                                column_valuator -= column_step; \
				} \
                                if (DO_X_STEP) \
				        column_step = -column_step
/* End of macro definition */


#define Y_CALC_INTENSITY_CENTER 0
#define Y_CALC_INTENSITY_NORMAL otherline[i]*line_valuator

#define WRITE_YPART(DO_Y_STEP, LINE_COLOR) \
			for (suby = 0; suby < ((DO_Y_STEP)? yfact >> 1 : 1); suby++) { \
				int column_valuator = column_step? 128 - (column_step >> 1) : 256; \
				int linecolor[4]; \
				int othercolumn[4]; \
				int i; \
				SIZETYPE wrcolor; \
				wrpos = sublinepos; \
                                alpha_wrpos = alpha_sublinepos; \
				for (i = 0; i < 4; i++) \
					linecolor[i] = LINE_COLOR; \
				/*-- left half --*/ \
				MAKE_PIXEL((x == 0), othercolumn, ctexel, src[-1]); \
				WRITE_XPART(X_CALC_INTENSITY_NORMAL, 1); \
				column_valuator -= column_step; \
				/*-- center --*/ \
				if (xfact & 1) { \
					WRITE_XPART(X_CALC_INTENSITY_CENTER, 0); \
				} \
				/*-- right half --*/ \
				MAKE_PIXEL((x+1 == pxm->index_xl), othercolumn, ctexel, src[+1]); \
				WRITE_XPART(X_CALC_INTENSITY_NORMAL, 1); \
				if (DO_Y_STEP) \
                                        line_valuator -= line_step; \
				sublinepos += pxm->xl * bytespp; \
				alpha_sublinepos += pxm->xl; \
			} \
			if (DO_Y_STEP) \
			        line_step = -line_step
/* End of macro definition */




void
FUNCNAME_LINEAR(gfx_mode_t *mode, gfx_pixmap_t *pxm, int scale)
{
	int xfact = mode->xfact;
	int yfact = mode->yfact;
	int line_step = (yfact < 2)? 0 : 256 / (yfact & ~1);
	int column_step = (xfact < 2)? 0 : 256 / (xfact & ~1);
	int bytespp = mode->bytespp;
	byte *src = pxm->index_data;
	byte *dest = pxm->data;
	byte *alpha_dest = pxm->alpha_map;
	int using_alpha = pxm->colors_nr < GFX_PIC_COLORS;
	int separate_alpha_map = (!mode->alpha_mask) && using_alpha;
	unsigned int masks[4], shifts[4], zero[3];
	int x,y;

	zero[0] = 255;
	zero[1] = zero[2] = 0;

	if (separate_alpha_map) {
		masks[3] = 0;
		shifts[3] = 24;
	}

        assert(bytespp == COPY_BYTES);
	assert(!PALETTE_MODE);

	masks[0] = mode->red_mask;
	masks[1] = mode->green_mask;
	masks[2] = mode->blue_mask;
	masks[3] = mode->alpha_mask;
	shifts[0] = mode->red_shift;
	shifts[1] = mode->green_shift;
	shifts[2] = mode->blue_shift;
	shifts[3] = mode->alpha_shift;

	if (separate_alpha_map && !alpha_dest)
		alpha_dest = pxm->alpha_map = malloc(pxm->index_xl * xfact * pxm->index_yl * yfact);

	for (y = 0; y < pxm->index_yl; y++) {
		byte *linepos = dest;
		byte *alpha_linepos = alpha_dest;

		for (x = 0; x < pxm->index_xl; x++) {
			int otherline[4]; /* the above line or the line below */
			int ctexel[4]; /* Current texel */
			int subx, suby;
			int j;
			int line_valuator = line_step? 128 - (line_step >> 1) : 256;
			byte *wrpos, *alpha_wrpos;
			byte *sublinepos = linepos;
			byte *alpha_sublinepos = alpha_linepos;

			ctexel[0] = ctexel[1] = ctexel[2] = ctexel[3] = 0;

#define MAKE_PIXEL(cond, rec, other, nr) \
			if ((cond) || (using_alpha && nr == 255)) { \
				rec[0] = other[0] - ctexel[0]; \
				rec[1] = other[1] - ctexel[1]; \
				rec[2] = other[2] - ctexel[2]; \
				rec[3] = 0xffff - ctexel[3]; \
			} else { \
				rec[0] = (EXTEND_COLOR(pxm->colors[nr].r) >> 16) - ctexel[0]; \
				rec[1] = (EXTEND_COLOR(pxm->colors[nr].g) >> 16) - ctexel[1]; \
				rec[2] = (EXTEND_COLOR(pxm->colors[nr].b) >> 16) - ctexel[2]; \
				rec[3] = 0 - ctexel[3]; \
			}

			MAKE_PIXEL(0, ctexel, zero, *src);

			/*-- Upper half --*/
			MAKE_PIXEL((y == 0), otherline, ctexel, src[-pxm->index_xl]);
			WRITE_YPART(1, Y_CALC_INTENSITY_NORMAL);

			if (yfact & 1) {
				WRITE_YPART(0, Y_CALC_INTENSITY_CENTER);
			}

			/*-- Lower half --*/
			line_valuator -= line_step;
			MAKE_PIXEL((y+1 == pxm->index_yl), otherline, ctexel, src[pxm->index_xl]);
			WRITE_YPART(1, Y_CALC_INTENSITY_NORMAL);

			src++;
			linepos += xfact * bytespp;
			alpha_linepos += xfact;
		}

		dest += pxm->xl * yfact * bytespp;
		alpha_dest += pxm->xl * yfact;
	}
}



/*----------------------*/
/*** Trilinear filter ***/
/*----------------------*/
/* Broken... */


#define MAKE_PIXEL_TRILINEAR(cond, rec, other, nr) \
			if ((cond) || (using_alpha && nr == 255)) { \
				rec[0] = other[0]; \
				rec[1] = other[1]; \
				rec[2] = other[2]; \
				rec[3] = 0xffff; \
			} else { \
				rec[0] = (EXTEND_COLOR(pxm->colors[nr].r) >> 16); \
				rec[1] = (EXTEND_COLOR(pxm->colors[nr].g) >> 16); \
				rec[2] = (EXTEND_COLOR(pxm->colors[nr].b) >> 16); \
				rec[3] = 0; \
			}

void
FUNCNAME_TRILINEAR(gfx_mode_t *mode, gfx_pixmap_t *pxm, int scale)
{
	int xfact = mode->xfact;
	int yfact = mode->yfact;
	int line_step = (yfact < 2)? 0 : 256 / yfact;
	int column_step = (xfact < 2)? 0 : 256 / xfact;
	int bytespp = mode->bytespp;
	byte *src = pxm->index_data;
	byte *dest = pxm->data;
	byte *alpha_dest = pxm->alpha_map;
	int using_alpha = pxm->colors_nr < GFX_PIC_COLORS;
	int separate_alpha_map = (!mode->alpha_mask) && using_alpha;
	unsigned int masks[4], shifts[4];
	unsigned int pixels[4][4]; 
	/* 0 1
	** 2 3 */
	unsigned int color[4];
	int x,y,j;

	if (separate_alpha_map) {
		masks[3] = 0;
		shifts[3] = 24;
	}

        assert(bytespp == COPY_BYTES);
	assert(!PALETTE_MODE);

	masks[0] = mode->red_mask;
	masks[1] = mode->green_mask;
	masks[2] = mode->blue_mask;
	masks[3] = mode->alpha_mask;
	shifts[0] = mode->red_shift;
	shifts[1] = mode->green_shift;
	shifts[2] = mode->blue_shift;
	shifts[3] = mode->alpha_shift;

	if (separate_alpha_map && !alpha_dest)
		alpha_dest = pxm->alpha_map = malloc(pxm->index_xl * xfact * pxm->index_yl * yfact);


	color[3] = 0xffffff;
	for (y = 0; y < pxm->index_yl - 1; y++) {
		int yc;
		int xbit = 0;

		MAKE_PIXEL_TRILINEAR(0, pixels[0], pixels[3], src[0]);
		MAKE_PIXEL_TRILINEAR(0, pixels[2], pixels[3], src[pxm->index_xl]);
		for (yc = 0; yc < yfact; yc++) {
			for (x = 0; x < pxm->index_xl - 1; x++) {
				int xc;

				xbit ^= 1;
				MAKE_PIXEL_TRILINEAR(0, pixels[xbit], pixels[3], src[1]);
				MAKE_PIXEL_TRILINEAR(0, pixels[2+xbit], pixels[3], src[pxm->index_xl + 1]);

				for (xc = 0; xc < xfact; xc++) {
					SIZETYPE wrcolor;
					int intensity;
					int i;

					intensity = (color[3] >> shifts[3]) & masks[3];
					for (i = 0; i < 2; i++)
						wrcolor |= (color[i] >> shifts[i]) & masks[i];

                                        if (separate_alpha_map)
                                                *alpha_dest++ = intensity >> 24;
					else
						wrcolor |= intensity;

					wrcolor <<= (EXTRA_BYTE_OFFSET * 8);
					memcpy(dest, &wrcolor, COPY_BYTES);
					dest += COPY_BYTES;
				}

				src++;
			}
		}
	}

}

#undef WRITE_YPART
#undef Y_CALC_INTENSITY_CENTER
#undef Y_CALC_INTENSITY_NORMAL
#undef WRITE_XPART
#undef X_CALC_INTENSITY_CENTER
#undef X_CALC_INTENSITY_NORMAL
#undef MAKE_PIXEL_TRILINEAR
#undef MAKE_PIXEL
#undef FUNCNAME
#undef FUNCNAME_LINEAR
#undef FUNCNAME_TRILINEAR
#undef SIZETYPE
#undef EXTEND_COLOR
