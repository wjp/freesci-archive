/*
 Copyright 2000, 2001, 2002
	Dan Potter. All rights reserved.

 This program is free software; you can redistribute it and/or modify
 it under the terms of the KOS License.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 KOS License (README.KOS) for more details.

 You should have received a copy of the KOS License along with this
 program; if not, please visit Cryptic Allusion DCDev at:

   http://dcdev.allusion.net/

*/

/* Modified by Walter van Niftrik <w.f.b.w.v.niftrik@stud.tue.nl> */

#ifndef __GP_H
#define __GP_H

#include <kos.h>
#include <math.h>

/* Floating-point Sin/Cos; 256 angles, -1.0 to 1.0 */
#define msin(angle) fsin((angle) * 2 * M_PI / 256)
#define mcos(angle) fcos((angle) * 2 * M_PI / 256)

/* bkg.c */
void bkg_setup();
void bkg_render();

/* texture.c */
extern pvr_ptr_t		util_texture;
extern pvr_poly_hdr_t		util_txr_hdr;
void setup_util_texture();

/* 3dutils.c */
void rotate(int zang, int xang, int yang, float *x, float *y, float *z);
void draw_poly_mouse(int ptrx, int ptry, float alpha);
void draw_poly_char(float x1, float y1, float z1, float a, float r, float g, float b, int c);
void draw_poly_strf(float x1, float y1, float z1, float a, float r, float g, float b, char *fmt, ...);
void draw_poly_box(float x1, float y1, float x2, float y2, float z,
		float a1, float r1, float g1, float b1,
		float a2, float r2, float g2, float b2);

/* songmenu.c */
int game_menu_render();

#endif	/* __GP_H */
