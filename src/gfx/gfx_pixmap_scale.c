/* Required defines:
** FUNCNAME: Function name
** SIZETYPE: Type used for each pixel
** EXTRA_BYTE_OFFSET: Extra source byte offset for copying (used on big-endian machines in 24 bit mode)
*/

#define EXTEND_COLOR(x) (((x) << 24) | ((x) << 16) | ((x) << 8) | (x))
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
			SIZETYPE col = result_colors[isalpha = *src++];

			isalpha = (isalpha == 255) && using_alpha;

			/* O(n) loops. There is an O(ln(n)) algorithm for this, but its slower for small n (which we're optimizing for here) */
			for (widthc = 0; widthc < xfact; widthc++) {
				memcpy(dest, &col + EXTRA_BYTE_OFFSET, sizeof(SIZETYPE));
				dest += sizeof(SIZETYPE);
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

#undef FUNCNAME
#undef SIZETYPE
#undef EXTEND_COLOR
