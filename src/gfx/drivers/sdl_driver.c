/***************************************************************************
 sdl_driver.h Copyright (C) 2001 Solomon Peachy


 This program may be modified and copied freely according to the terms of
 the GNU general public license (GPL), as long as the above copyright
 notice and the licensing information contained herein are preserved.

 Please refer to www.gnu.org for licensing details.

 This work is provided AS IS, without warranty of any kind, expressed or
 implied, including but not limited to the warranties of merchantibility,
 noninfringement, and fitness for a specific purpose. The author will not
 be held liable for any damage caused by this work or derivatives of it.

 By using this source code, you agree to the licensing terms as stated
 above.


 Please contact the maintainer for bug reports or inquiries.

 Current Maintainer:
     Solomon Peachy <pizza@shaftnet.org>

***************************************************************************/

#include <gfx_driver.h>
#ifdef HAVE_SDL
#include <gfx_tools.h>

#ifndef _MSC_VER
#	include <sys/time.h>
#	include <SDL/SDL.h>
#else
#	include <winsock2.h>
#	include <SDL.h>
#endif

#ifndef SDL_DISABLE
#  define SDL_DISABLE 0
#endif
#ifndef SDL_ALPHA_OPAQUE
#  define SDL_ALPHA_OPAQUE 255
#endif

#define SCI_SDL_HANDLE_NORMAL 0
#define SCI_SDL_HANDLE_GRABBED 1

#define SCI_SDL_SWAP_CTRL_CAPS (1 << 0)
#define SCI_SDL_FULLSCREEN (1 << 2)

int string_truep(char *value); 
int flags = 0;

struct _sdl_state {
  int used_bytespp;
  gfx_pixmap_t *priority[2];
  SDL_Color colors[256];
  SDL_Surface *visual[3];
  SDL_Surface *primary;
  int buckystate;
  byte *pointer_data[2];
  int alpha_mask;
  int SDL_alpha_shift;
  int SDL_alpha_loss;
};

#define S ((struct _sdl_state *)(drv->state))

#define XFACT drv->mode->xfact
#define YFACT drv->mode->yfact

#define DEBUGB if (drv->debug_flags & GFX_DEBUG_BASIC && ((debugline = __LINE__))) sdlprintf
#define DEBUGU if (drv->debug_flags & GFX_DEBUG_UPDATES && ((debugline = __LINE__))) sdlprintf
#define DEBUGPXM if (drv->debug_flags & GFX_DEBUG_PIXMAPS && ((debugline = __LINE__))) sdlprintf
#define DEBUGPTR if (drv->debug_flags & GFX_DEBUG_POINTER && ((debugline = __LINE__))) sdlprintf
#define ERROR if ((debugline = __LINE__)) sdlprintf

#define ALPHASURFACE (S->used_bytespp == 4)

static int debugline = 0;

static void
sdlprintf(char *fmt, ...)
{
  va_list argp;
  fprintf(stderr,"GFX-SDL %d:", debugline);
  va_start(argp, fmt);
  vfprintf(stderr, fmt, argp);
  va_end(argp);
}

static void
sdlerror(gfx_driver_t *drv, int line)
{
  sdlprintf("Error in line %d\n", line);
}

static int
sdl_set_parameter(struct _gfx_driver *drv, char *attribute, char *value)
{
  if (!strncmp(attribute, "swap_ctrl_caps", 15) ||
      !strncmp(attribute, "swap_caps_ctrl", 15)) {
    if (string_truep(value))
      flags |= SCI_SDL_SWAP_CTRL_CAPS;
    else
      flags &= ~SCI_SDL_SWAP_CTRL_CAPS;
    return GFX_OK;
  }

  if (!strncmp(attribute, "fullscreen", 11)) {
    if (string_truep(value))
      flags |= SCI_SDL_FULLSCREEN;
    else
      flags &= ~SCI_SDL_FULLSCREEN;
  
    return GFX_OK;
  }

  ERROR("Attempt to set sdl parameter \"%s\" to \"%s\"\n", attribute, value);
  return GFX_ERROR;
}

static int
sdl_init_specific(struct _gfx_driver *drv, int xfact, int yfact, int bytespp)
{
  int red_shift, green_shift, blue_shift, alpha_shift;
  int xsize = xfact * 320;
  int ysize = yfact * 200;

  int i;
  
  if (!S)
    S = malloc(sizeof(struct _sdl_state));
  if (!S)
    return GFX_FATAL;
    
  if (xfact < 1 || yfact < 1 || bytespp < 1 || bytespp > 4) {
    ERROR("Internal error: Attempt to open window w/ scale factors (%d,%d) and bpp=%d!\n",
	  xfact, yfact, bytespp);
  }

  S->primary = NULL;

  i = SDL_HWSURFACE | SDL_SWSURFACE | SDL_HWPALETTE | SDL_DOUBLEBUF;
  if (flags & SCI_SDL_FULLSCREEN) {
    i |= SDL_FULLSCREEN;
#ifdef _MSC_VER
    ysize = yfact * 240;
#endif
  }

  S->primary = SDL_SetVideoMode(xsize, ysize, bytespp << 3, i);

  if (!S->primary) {
    ERROR("Could not set up a primary SDL surface!\n");
    return GFX_FATAL;
  }

  if (S->primary->format->BytesPerPixel != bytespp) {
    ERROR("Could not set up a primary SDL surface of depth %d bpp!\n",bytespp);
    SDL_FreeSurface(S->primary);
    S->primary = NULL;
    return GFX_FATAL;
  }

  S->used_bytespp = bytespp;

  printf("Using primary SDL surface of %d,%d @%d bpp (%04x)\n",  
	 xsize, ysize, bytespp << 3, S->primary);

  /*  if (S->primary->format->BytesPerPixel == 4) {    
    S->alpha_mask = 0xff000000;
    S->SDL_alpha_shift = 24;
    S->SDL_alpha_loss = 0;
    alpha_shift = 0;
    } else { */
    S->alpha_mask = S->primary->format->Amask;
    S->SDL_alpha_shift = S->primary->format->Ashift;
    S->SDL_alpha_loss = S->primary->format->Aloss;
    alpha_shift = bytespp << 3;
    /*   }*/

  /* clear palette */
  for (i = 0; i < 256; i++) {
    S->colors[i].r = 0;
    S->colors[i].g = 0;
    S->colors[i].b = 0;
  }
  if (bytespp == 1) 
    SDL_SetColors(S->primary, S->colors, 0, 256);

  /* create an input event mask */
  SDL_EventState(SDL_ACTIVEEVENT, SDL_IGNORE);
  SDL_EventState(SDL_MOUSEMOTION, SDL_IGNORE);
  SDL_EventState(SDL_VIDEORESIZE, SDL_IGNORE);
  SDL_EventState(SDL_KEYUP, SDL_IGNORE);

  SDL_WM_SetCaption("FreeSCI", "freesci");

  SDL_ShowCursor(SDL_DISABLE);
  S->pointer_data[0] = NULL;
  S->pointer_data[1] = NULL;

  S->buckystate = 0;

  if (bytespp == 1)
    red_shift = green_shift = blue_shift = alpha_shift = 0;
  else {
    red_shift = 24 - S->primary->format->Rshift + S->primary->format->Rloss;
    green_shift = 24 - S->primary->format->Gshift + S->primary->format->Gloss;
    blue_shift = 24 - S->primary->format->Bshift + S->primary->format->Bloss;
  }

  printf("%08x %08x %08x %08x %d/%d=%d %d/%d=%d %d/%d=%d %d/%d=%d\n",
	 S->primary->format->Rmask, 
	 S->primary->format->Gmask,
	 S->primary->format->Bmask, 
	 S->alpha_mask,
	 /*	 S->primary->format->Amask,*/
	 S->primary->format->Rshift,
	 S->primary->format->Rloss,
	 red_shift,
	 S->primary->format->Gshift,
	 S->primary->format->Gloss,
	 green_shift,
	 S->primary->format->Bshift,
	 S->primary->format->Bloss,
	 blue_shift,
	 S->SDL_alpha_shift,
	 S->SDL_alpha_loss,
	 /*
	 S->primary->format->Ashift,
	 S->primary->format->Aloss, */
	 alpha_shift);

  for (i = 0; i < 2; i++) {
    S->priority[i] = gfx_pixmap_alloc_index_data(gfx_new_pixmap(xsize, ysize, GFX_RESID_NONE, -i, -777));
    if (!S->priority[i]) {
      ERROR("Out of memory: Could not allocate priority maps! (%dx%d)\n",
	    xsize, ysize);
      return GFX_FATAL;
    }
  }

  /* create the visual buffers */
  for (i = 0; i < 3; i++) { 
    S->visual[i] = NULL;
    S->visual[i] = SDL_CreateRGBSurface(SDL_SRCALPHA,
					/* SDL_HWSURFACE | SDL_SWSURFACE, */
					xsize, ysize, 
					bytespp << 3, 
					S->primary->format->Rmask, 
					S->primary->format->Gmask,
					S->primary->format->Bmask, 
					S->alpha_mask);
    if (S->visual[i] == NULL) {
      ERROR("Could not set up visual buffers!\n");
      return GFX_FATAL;
    }

    if (ALPHASURFACE)
     SDL_SetAlpha(S->visual[i],SDL_SRCALPHA,SDL_ALPHA_OPAQUE);

    if (SDL_FillRect(S->primary, NULL, SDL_MapRGB(S->primary->format, 0,0,0)))
      ERROR("Couldn't fill backbuffer!\n");
  }
  
  drv->mode = gfx_new_mode(xfact, yfact, bytespp,
			   S->primary->format->Rmask, 
			   S->primary->format->Gmask,
			   S->primary->format->Bmask,
			   S->alpha_mask,
			   red_shift, green_shift, blue_shift, alpha_shift,
			   (bytespp == 1)? 256 : 0, 0); /*GFX_MODE_FLAG_REVERSE_ALPHA);*/
  
  return GFX_OK;
}

static int
sdl_init(struct _gfx_driver *drv)
{
  int depth = 0;
  int i;

  if (SDL_Init(SDL_INIT_VIDEO)) {
    DEBUGB("Failed to init SDL\n");
    return GFX_FATAL;
  }
  SDL_EnableUNICODE(SDL_ENABLE);

  i = SDL_HWSURFACE | SDL_SWSURFACE | SDL_HWPALETTE | SDL_DOUBLEBUF;
  if (flags & SCI_SDL_FULLSCREEN) {
    i |= SDL_FULLSCREEN;
  }

  depth = SDL_VideoModeOK(640,400, 32, i);
  if (depth && (! sdl_init_specific(drv, 2, 2, depth >> 3 )))
    return GFX_OK;

  DEBUGB("Failed to find visual!\n");
  return GFX_FATAL;
}

static void
sdl_exit(struct _gfx_driver *drv)
{
  int i;
  if (S) {
    for (i = 0; i < 2; i++) {
      gfx_free_pixmap(drv, S->priority[i]);
      S->priority[i] = NULL;
    }

    for (i = 0; i < 3; i++) {
      SDL_FreeSurface(S->visual[i]);
      S->visual[i] = NULL;
    }

    SDL_FreeCursor(SDL_GetCursor());

    for (i = 0; i < 2; i++)
      if (S->pointer_data[i]) {
	free(S->pointer_data[i]);
	S->pointer_data[i] = NULL;
      }
    
  }
  SDL_Quit();
}


  /*** Drawing operations ***/

static Uint32 
sdl_map_color(gfx_driver_t *drv, gfx_color_t color)
{

  if (drv->mode->palette)
    return color.visual.global_index;
  
  return SDL_MapRGBA(S->visual[0]->format,
		     color.visual.r,
		     color.visual.g,
		     color.visual.b,
		     255 - color.alpha);
}


/* This code shamelessly lifted from the SDL_gfxPrimitives package */
static void lineColor(SDL_Surface *dst, Sint16 x1, Sint16 y1, Sint16 x2, Sint16 y2, Uint32 color)
{
  int pixx, pixy;
  int x,y;
  int dx,dy;
  int ax,ay;
  int sx,sy;
  int swaptmp;
  Uint8 *pixel;
  Uint8 *colorptr;

  dx = x2 - x1;
  dy = y2 - y1;
  sx = (dx >= 0) ? 1 : -1;
  sy = (dy >= 0) ? 1 : -1;

  dx = sx * dx + 1;
  dy = sy * dy + 1;
  pixx = dst->format->BytesPerPixel;
  pixy = dst->pitch;
  pixel = ((Uint8*)dst->pixels) + pixx * (int)x1 + pixy * (int)y1;
  pixx *= sx;
  pixy *= sy;
  if (dx < dy) {
   swaptmp = dx; dx = dy; dy = swaptmp;
   swaptmp = pixx; pixx = pixy; pixy = swaptmp;
  }

/* Draw */
  x=0;
  y=0;
  switch(dst->format->BytesPerPixel) {
   case 1:
    for(; x < dx; x++, pixel += pixx) {
     *pixel = color;
     y += dy; 
     if (y >= dx) {
      y -= dx; pixel += pixy;
     }
    }
    break;
   case 2:
    for (; x < dx; x++, pixel += pixx) {
     *(Uint16*)pixel = color;
     y += dy; 
     if (y >= dx) {
      y -= dx; 
      pixel += pixy;
     }
    }
    break;
   case 3:
    for(; x < dx; x++, pixel += pixx) {
     if(SDL_BYTEORDER == SDL_BIG_ENDIAN) {
      pixel[0] = (color >> 16) & 0xff;
      pixel[1] = (color >> 8) & 0xff;
      pixel[2] = color & 0xff;
     } else {
      pixel[0] = color & 0xff;
      pixel[1] = (color >> 8) & 0xff;
      pixel[2] = (color >> 16) & 0xff;
     }
     y += dy; 
     if (y >= dx) {
      y -= dx; 
      pixel += pixy;
   }
    }
    break;
   case 4: 
     for(; x < dx; x++, pixel += pixx) {
      *(Uint32*)pixel = color;
      y += dy; 
      if (y >= dx) {
       y -= dx; 
       pixel += pixy;
      }
     }
     break;
   default:
     fprintf(stderr, "invalid depth\n");
  }

}

static int
sdl_draw_line(struct _gfx_driver *drv, rect_t line, gfx_color_t color,
	      gfx_line_mode_t line_mode, gfx_line_style_t line_style)
{
  Uint32 scolor;
  int linewidth = (line_mode == GFX_LINE_MODE_FINE)? 1:
    (XFACT + YFACT) >> 1;
  
  if (color.mask & GFX_MASK_VISUAL) {
    int xc, yc;
    rect_t newline;

    scolor = sdl_map_color(drv, color);
    newline.xl = line.x;
    newline.yl = line.y;

    /* XXXX line_style = 
       (line_style == GFX_LINE_STYLE_NORMAL)? LineSolid : LineOnOffDash; 
    and set GFX_CAPABILITY_STIPPLED_LINES */

    for (xc = -linewidth; xc++; xc <= linewidth)
      for (yc = -linewidth; yc++; yc <= linewidth) {
	newline.x = line.x + xc;
	newline.y = line.y + yc;
	newline.xl = line.x + line.xl + xc;
	newline.yl = line.y + line.yl + yc;
	lineColor(S->visual[1], newline.x, newline.y,
		  newline.xl, newline.yl, scolor);
      }
  }

  if (color.mask & GFX_MASK_PRIORITY) {
    int xc, yc;
    rect_t newline;
    
    newline.xl = line.xl;
    newline.yl = line.yl;
    
    linewidth--;
    for (xc = -linewidth; xc++; xc <= linewidth)
      for (yc = -linewidth; yc++; yc <= linewidth) {
	newline.x = line.x + xc;
	newline.y = line.y + yc;
	gfx_draw_line_pixmap_i(S->priority[0], newline, color.priority);
      }
  }

  return GFX_OK;
}

static int
sdl_draw_filled_rect(struct _gfx_driver *drv, rect_t rect,
		     gfx_color_t color1, gfx_color_t color2,
		     gfx_rectangle_fill_t shade_mode)
{
  Uint32 color;
  SDL_Rect srect;

  if (color1.mask & GFX_MASK_VISUAL) {
    color = sdl_map_color(drv, color1);

    srect.x = rect.x;
    srect.y = rect.y;
    srect.w = rect.xl;
    srect.h = rect.yl;
      
    if (SDL_FillRect(S->visual[1], &srect, color))
      ERROR("Can't fill rect");
  }
  
  if (color1.mask & GFX_MASK_PRIORITY)
    gfx_draw_box_pixmap_i(S->priority[0], rect, color1.priority);
  
  return GFX_OK;
}

  /*** Pixmap operations ***/

static int
sdl_register_pixmap(struct _gfx_driver *drv, gfx_pixmap_t *pxm)
{
  if (pxm->internal.info) {
    ERROR("Attempt to register pixmap twice!\n");
    return GFX_ERROR;
  }

  pxm->internal.info = SDL_CreateRGBSurfaceFrom(pxm->data, pxm->xl, pxm->yl,
						S->used_bytespp << 3,
						S->used_bytespp * pxm->xl,
						S->primary->format->Rmask, 
						S->primary->format->Gmask,
						S->primary->format->Bmask, 
						S->alpha_mask);

  if (ALPHASURFACE)
      SDL_SetAlpha(pxm->internal.info, SDL_SRCALPHA,SDL_ALPHA_OPAQUE);

  pxm->internal.handle = SCI_SDL_HANDLE_NORMAL;

  DEBUGPXM("Registered surface %d/%d/%d at %p (%dx%d)\n", pxm->ID, pxm->loop, pxm->cel,
	   pxm->internal.info, pxm->xl, pxm->yl);
  return GFX_OK;
}

static int
sdl_unregister_pixmap(struct _gfx_driver *drv, gfx_pixmap_t *pxm)
{
  DEBUGPXM("Freeing surface %d/%d/%d at %p\n", pxm->ID, pxm->loop, pxm->cel,
	   pxm->internal.info);
  
  if (!pxm->internal.info) {
    ERROR("Attempt to unregister pixmap twice!\n");
    return GFX_ERROR;
  }

  SDL_FreeSurface((SDL_Surface *) pxm->internal.info);
  pxm->internal.info = NULL;
  if (pxm->internal.handle != SCI_SDL_HANDLE_GRABBED)
    free(pxm->data);
  pxm->data = NULL; 
  return GFX_OK;
}

static int
sdl_draw_pixmap(struct _gfx_driver *drv, gfx_pixmap_t *pxm, int priority,
		rect_t src, rect_t dest, gfx_buffer_t buffer)
{
  int bufnr = (buffer == GFX_BUFFER_STATIC)? 2:1;
  int pribufnr = bufnr -1;
  
  SDL_Surface *temp;
  SDL_Rect srect;
  SDL_Rect drect;

  if (dest.xl != src.xl || dest.yl != src.yl) {
    ERROR("Attempt to scale pixmap (%dx%d)->(%dx%d): Not supported\n",
	  src.xl, src.yl, dest.xl, dest.yl);
    return GFX_ERROR;
  }

  srect.x = src.x;
  srect.y = src.y;
  srect.w = src.xl;
  srect.h = src.yl;
  drect.x = dest.x;
  drect.y = dest.y;
  drect.w = dest.xl;
  drect.h = dest.yl;

  DEBUGU("Drawing %d (%d,%d)(%dx%d) onto (%d,%d)\n", pxm, srect.x, srect.y,
	 srect.w, srect.h, drect.x, drect.y);
  
  if (pxm->internal.handle == SCI_SDL_HANDLE_GRABBED) {
    if (SDL_BlitSurface((SDL_Surface *)pxm->internal.info, &srect , 
			S->visual[bufnr], &drect )) {
      ERROR("blt failed");
      return GFX_ERROR;
    }
    return GFX_OK;
  }

  temp = SDL_CreateRGBSurface(SDL_SWSURFACE, drect.w, drect.h,
			      S->used_bytespp << 3,
			      S->primary->format->Rmask, 
			      S->primary->format->Gmask,
			      S->primary->format->Bmask, 
			      S->alpha_mask);

  if (ALPHASURFACE)
    SDL_SetAlpha(temp, SDL_SRCALPHA,SDL_ALPHA_OPAQUE);

  if (!temp) {
    ERROR("Failed to allocate SDL surface");
    return GFX_ERROR;
  }  

  srect.x = dest.x;
  srect.y = dest.y;
  drect.x = 0;
  drect.y = 0;

  if(SDL_BlitSurface(S->visual[bufnr], &srect, temp, &drect))
    ERROR("blt failed");

  gfx_crossblit_pixmap(drv->mode, pxm, priority, src, dest,
		       (byte *) temp->pixels, temp->pitch,
		       S->priority[pribufnr]->index_data,
		       S->priority[pribufnr]->index_xl, 1,
		       GFX_CROSSBLIT_FLAG_DATA_IS_HOMED);

  srect.x = 0;
  srect.y = 0;
  drect.x = dest.x;
  drect.y = dest.y;
  
  if(SDL_BlitSurface(temp, &srect, S->visual[bufnr], &drect))
    ERROR("blt failed");

  SDL_FreeSurface(temp);
  return GFX_OK;
}

static int
sdl_grab_pixmap(struct _gfx_driver *drv, rect_t src, gfx_pixmap_t *pxm,
		gfx_map_mask_t map)
{


  if (src.x < 0 || src.y < 0) {
    ERROR("Attempt to grab pixmap from invalid coordinates (%d,%d)\n", src.x, src.y);
    return GFX_ERROR;
  }

  if (!pxm->data) {
    ERROR("Attempt to grab pixmap to unallocated memory\n");
    return GFX_ERROR;
  }
  switch (map) {
    
  case GFX_MASK_VISUAL: {
    SDL_Rect srect, drect;
    SDL_Surface *temp;    

    pxm->xl = src.xl;
    pxm->yl = src.yl;
    temp = SDL_CreateRGBSurface(SDL_SWSURFACE, src.xl, src.yl,
				S->used_bytespp << 3,
				S->primary->format->Rmask, 
				S->primary->format->Gmask,
				S->primary->format->Bmask, 
				S->alpha_mask);

  if (ALPHASURFACE)
      SDL_SetAlpha(temp, SDL_SRCALPHA,SDL_ALPHA_OPAQUE);

    if (!temp) {
      ERROR("Failed to allocate SDL surface");
      return GFX_ERROR;
    }

    srect.x = src.x;
    srect.y = src.y;
    srect.w = src.xl;
    srect.h = src.yl;
    drect.x = 0;
    drect.y = 0;
    drect.w = src.xl;
    drect.h = src.yl;

    if (SDL_BlitSurface(S->visual[1], &srect, temp, &drect))
      ERROR("grab_pixmap:  grab blit failed!\n");

    pxm->internal.info = temp;
    pxm->internal.handle = SCI_SDL_HANDLE_GRABBED;
    pxm->flags |= GFX_PIXMAP_FLAG_INSTALLED | GFX_PIXMAP_FLAG_EXTERNAL_PALETTE | GFX_PIXMAP_FLAG_PALETTE_SET;
    free(pxm->data);
    pxm->data = (byte *) temp->pixels;

    DEBUGPXM("Grabbed surface %p (%dx%d)(%dx%d)\n",
	     pxm->internal.info, srect.x, srect.y, pxm->xl, pxm->yl);

    break; 
  }
    
  case GFX_MASK_PRIORITY:
    ERROR("FIXME: priority map grab not implemented yet!\n");
    break;
    
  default:
    ERROR("Attempt to grab pixmap from invalid map 0x%02x\n", map);
    return GFX_ERROR;
  }

  return GFX_OK;
}


  /*** Buffer operations ***/

static int
sdl_update(struct _gfx_driver *drv, rect_t src, point_t dest, gfx_buffer_t buffer)
{
  int data_source = (buffer == GFX_BUFFER_BACK)? 2 : 1;
  int data_dest = data_source - 1;
  SDL_Rect srect, drect;
  
  if (src.x != dest.x || src.y != dest.y) {
    DEBUGU("Updating %d (%d,%d)(%dx%d) to (%d,%d) on %d\n", buffer, src.x, src.y,
	   src.xl, src.yl, dest.x, dest.y, data_dest);
  } else {
    DEBUGU("Updating %d (%d,%d)(%dx%d) to %d\n", buffer, src.x, src.y, src.xl, src.yl, data_dest);
  }

  srect.x = src.x;
  srect.y = src.y;
  srect.w = src.xl;
  srect.h = src.yl;
  drect.x = dest.x;
  drect.y = dest.y;
  drect.w = src.xl;
  drect.h = src.yl;

  switch (buffer) {
  case GFX_BUFFER_BACK:
    if (SDL_BlitSurface(S->visual[data_source], &srect, 
			S->visual[data_dest], &drect))
      ERROR("surface update failed!\n");
    
    if ((src.x == dest.x) && (src.y == dest.y)) 
      gfx_copy_pixmap_box_i(S->priority[0], S->priority[1], src);
    break;
  case GFX_BUFFER_FRONT:
    if (SDL_BlitSurface(S->visual[data_source], &srect, S->primary, &drect))
      ERROR("primary surface update failed!\n");
    SDL_Flip(S->primary);
    break;
  default:
    GFXERROR("Invalid buffer %d in update!\n", buffer);
    return GFX_ERROR;
  }

  return GFX_OK;
}

static int
sdl_set_static_buffer(struct _gfx_driver *drv, gfx_pixmap_t *pic, gfx_pixmap_t *priority)
{

  if (!pic->internal.info) {
    ERROR("Attempt to set static buffer with unregisterd pixmap!\n");
    return GFX_ERROR;
  }
  SDL_BlitSurface((SDL_Surface *)pic->internal.info, NULL, 
		  S->visual[2], NULL);

  gfx_copy_pixmap_box_i(S->priority[1], priority, gfx_rect(0, 0, 320*XFACT, 200*YFACT));

  return GFX_OK;
}

  /*** Palette operations ***/

static int
sdl_set_palette(struct _gfx_driver *drv, int index, byte red, byte green, byte blue)
{
  S->colors[index].r = red;
  S->colors[index].g = green;
  S->colors[index].b = blue;

  SDL_SetColors(S->primary, S->colors, 0, 256);
  return GFX_OK;
}


  /*** Mouse pointer operations ***/

byte *
sdl_create_cursor_rawdata(gfx_driver_t *drv, gfx_pixmap_t *pointer, int mode)
{
  int linewidth = (pointer->xl + 7) >> 3;
  int lines = pointer->yl;
  int xc, yc;
  byte *data = calloc(linewidth, lines);
  byte *linebase = data, *pos;
  byte *src = pointer->index_data;
  
  for (yc = 0; yc < pointer->index_yl; yc++) {
    int scalectr;
    int bitc = 7;
    pos = linebase;
    
    for (xc = 0; xc < pointer->index_xl; xc++) {
      int draw = mode?
	(*src == 0) : (*src < 255);
      for (scalectr = 0; scalectr < XFACT; scalectr++) {
	if (draw)
	  *pos |= (1 << bitc);
	bitc--;
	if (bitc < 0) {
	  bitc = 7;
	  pos++;
	}
      }
      src++;
    }
    for (scalectr = 1; scalectr < YFACT; scalectr++)
      memcpy(linebase + linewidth * scalectr, linebase, linewidth);
    linebase += linewidth * YFACT;
  }
  return data;
}


static SDL_Cursor
*sdl_create_cursor_data(gfx_driver_t *drv, gfx_pixmap_t *pointer)
{
  byte *visual_data, *mask_data;
  
  S->pointer_data[0] = visual_data = sdl_create_cursor_rawdata(drv, pointer, 1);
  S->pointer_data[1] = mask_data = sdl_create_cursor_rawdata(drv, pointer, 0);

  return SDL_CreateCursor(visual_data, mask_data, 
			  pointer->xl, pointer->yl,
			  pointer->xoffset, pointer->yoffset);
  
}

static int sdl_set_pointer (struct _gfx_driver *drv, gfx_pixmap_t *pointer)
{
  SDL_Cursor *cursor;
  int i;
  
  if (pointer == NULL)
    SDL_ShowCursor(SDL_DISABLE);
  else {
    for (i = 0; i < 2; i++)
      if (S->pointer_data[i]) {
	free(S->pointer_data[i]);
	S->pointer_data[i] = NULL;
      }
    SDL_FreeCursor(SDL_GetCursor());
    SDL_SetCursor(sdl_create_cursor_data(drv, pointer));
    SDL_ShowCursor(SDL_ENABLE);
  }  

  return 0;
}

  /*** Event management ***/

int
sdl_map_key(gfx_driver_t *drv, SDL_keysym keysym)
{
  SDLKey skey = keysym.sym;
  int rkey = keysym.unicode & 0x7f;

  if ((skey >= SDLK_a) && (skey <= SDLK_z))
    return ('a' + (skey - SDLK_a));

  if ((skey >= SDLK_0) && (skey <= SDLK_9))
    return ('0' + (skey - SDLK_0));
  
  if (flags & SCI_SDL_SWAP_CTRL_CAPS) {
    switch (skey) {
    case SDLK_LCTRL: skey = SDLK_CAPSLOCK; break;
    case SDLK_CAPSLOCK: skey = SDLK_LCTRL; break;
    }
  }

  switch (skey) {
    /* XXXX catch KMOD_NUM for KP0-9 */
  case SDLK_BACKSPACE: return SCI_K_BACKSPACE;
  case SDLK_TAB: return 9;
  case SDLK_ESCAPE: return SCI_K_ESC;
  case SDLK_RETURN:
  case SDLK_KP_ENTER: return SCI_K_ENTER;
  case SDLK_KP_PERIOD: return SCI_K_DELETE;
  case SDLK_KP0:
  case SDLK_INSERT: return SCI_K_INSERT;
  case SDLK_KP1:
  case SDLK_END: return SCI_K_END;
  case SDLK_KP2:
  case SDLK_DOWN: return SCI_K_DOWN;
  case SDLK_KP3:
  case SDLK_PAGEDOWN: return SCI_K_PGDOWN;
  case SDLK_KP4:
  case SDLK_LEFT: return SCI_K_LEFT;
  case SDLK_KP5: return SCI_K_CENTER;
  case SDLK_KP6:
  case SDLK_RIGHT: return SCI_K_RIGHT;
  case SDLK_KP7:
  case SDLK_HOME: return SCI_K_HOME;
  case SDLK_KP8:
  case SDLK_UP: return SCI_K_UP;
  case SDLK_KP9:
  case SDLK_PAGEUP: return SCI_K_PGUP;
    
  case SDLK_F1: return SCI_K_F1;
  case SDLK_F2: return SCI_K_F2;
  case SDLK_F3: return SCI_K_F3;
  case SDLK_F4: return SCI_K_F4;
  case SDLK_F5: return SCI_K_F5;
  case SDLK_F6: return SCI_K_F6;
  case SDLK_F7: return SCI_K_F7;
  case SDLK_F8: return SCI_K_F8;
  case SDLK_F9: return SCI_K_F9;
  case SDLK_F10: return SCI_K_F10;

  case SDLK_LCTRL:
  case SDLK_RCTRL:
  case SDLK_LALT:
  case SDLK_RALT:
  case SDLK_CAPSLOCK:
  case SDLK_SCROLLOCK:
  case SDLK_NUMLOCK:
  case SDLK_LSHIFT:
  case SDLK_RSHIFT:  return 0;
    
  case SDLK_PLUS:
  case SDLK_KP_PLUS: return '+';
  case SDLK_SLASH:
  case SDLK_KP_DIVIDE: return '/';
  case SDLK_MINUS:
  case SDLK_KP_MINUS: return '-';
  case SDLK_ASTERISK:
  case SDLK_KP_MULTIPLY: return '*';
  case SDLK_EQUALS:
  case SDLK_KP_EQUALS: return '=';

  case SDLK_COMMA:
  case SDLK_PERIOD:
  case SDLK_BACKSLASH:
  case SDLK_SEMICOLON:
  case SDLK_QUOTE:
  case SDLK_LEFTBRACKET:
  case SDLK_RIGHTBRACKET:
  case SDLK_LESS:
  case SDLK_GREATER:
  case SDLK_SPACE:  return rkey; 

  case SDLK_BACKQUOTE:
    if (keysym.mod & KMOD_CTRL)
      return '`';
    else
      return rkey;    
  }

  sciprintf("Unknown SDL keysym: %04x (%d) \n", skey, rkey);
  return 0;
}


void
sdl_fetch_event(gfx_driver_t *drv, long wait_usec, sci_event_t *sci_event)
{
  SDL_Event event;
  int x_button_xlate[] = {0, 1, 3, 2, 4, 5};
  struct timeval ctime, timeout_time, sleep_time;
  int usecs_to_sleep;
  int time_sec, time_usec;
  
  sci_gettime(&time_sec, &time_usec);
  timeout_time.tv_sec = time_sec;
  timeout_time.tv_usec = time_usec;
  timeout_time.tv_usec += wait_usec;
  
  /* Calculate wait time */
  timeout_time.tv_sec += (timeout_time.tv_usec / 1000000);
  timeout_time.tv_usec %= 1000000;
  
  do {
    int redraw_pointer_request = 0;

    while (SDL_PollEvent(&event)) {

      switch (event.type) {
      case SDL_KEYDOWN: {
	int modifiers = event.key.keysym.mod;
	sci_event->type = SCI_EVT_KEYBOARD;
	
	S->buckystate = (((modifiers & KMOD_CAPS)? SCI_EVM_LSHIFT | SCI_EVM_RSHIFT : 0)
			 | ((modifiers & KMOD_CTRL)? SCI_EVM_CTRL : 0)
			 | ((modifiers & KMOD_ALT)? SCI_EVM_ALT : 0)
			 | ((modifiers & KMOD_NUM) ? SCI_EVM_NUMLOCK : 0)
			 | ((modifiers & KMOD_RSHIFT)? SCI_EVM_RSHIFT : 0)
			 | ((modifiers & KMOD_LSHIFT)? SCI_EVM_LSHIFT : 0));

	sci_event->buckybits = S->buckystate;
	sci_event->data = sdl_map_key(drv, event.key.keysym);
	if (sci_event->data) 
	  return;
	break; }
      case SDL_MOUSEBUTTONDOWN:
	sci_event->type = SCI_EVT_MOUSE_PRESS;
	sci_event->buckybits = S->buckystate;
	sci_event->data = event.button.button - 1;
	drv->pointer_x = event.button.x;
	drv->pointer_y = event.button.y;
	return;
      case SDL_MOUSEBUTTONUP:
	sci_event->type = SCI_EVT_MOUSE_RELEASE;
	sci_event->buckybits = S->buckystate;
	sci_event->data = event.button.button - 1;
	drv->pointer_x = event.button.x;
	drv->pointer_y = event.button.y;
	return;
      case SDL_MOUSEMOTION:
	drv->pointer_x = event.motion.x;
	drv->pointer_y = event.motion.y;
	break;
      case SDL_QUIT:
	sci_event->type = SCI_EVT_QUIT;
	return;
	break;
      default:
	ERROR("Received unhandled SDL event %04x\n", event.type);
      }
    }

    sci_gettime(&time_sec, &time_usec);
    ctime.tv_sec = time_sec;
    ctime.tv_usec = time_usec;
    
    usecs_to_sleep = (timeout_time.tv_sec > ctime.tv_sec)? 1000000 : 0;
    usecs_to_sleep += timeout_time.tv_usec - ctime.tv_usec;
    if (ctime.tv_sec > timeout_time.tv_sec) usecs_to_sleep = -1;
        
    if (usecs_to_sleep > 0) {
      if (usecs_to_sleep > 10000)
	usecs_to_sleep = 10000; /* Sleep for a maximum of 10 ms */
      
      sleep_time.tv_usec = usecs_to_sleep;
      sleep_time.tv_sec = 0;
      
      select(0, NULL, NULL, NULL, &sleep_time); /* Sleep. */
    }
    
  } while (usecs_to_sleep >= 0);
  
  if (sci_event)
    sci_event->type = SCI_EVT_NONE; /* No event. */
}

static sci_event_t
sdl_get_event(struct _gfx_driver *drv)
{
	sci_event_t input;

	sdl_fetch_event(drv, 0, &input);
	return input;
}


static int
sdl_usec_sleep(struct _gfx_driver *drv, int usecs)
{
  struct timeval ctime;
  if (usecs > 10000)
    usecs = 10000;
  
  ctime.tv_sec = 0;
  ctime.tv_usec = usecs;
  
  select(0, NULL, NULL, NULL, &ctime); /* Sleep. */

  return GFX_OK;
}

gfx_driver_t
gfx_driver_sdl = {
	"sdl",
	"0.1",
	NULL,
	0, 0,
	GFX_CAPABILITY_MOUSE_SUPPORT | GFX_CAPABILITY_MOUSE_POINTER |
	GFX_CAPABILITY_PIXMAP_REGISTRY | GFX_CAPABILITY_PIXMAP_GRABBING,
	0, /*GFX_DEBUG_POINTER | GFX_DEBUG_UPDATES | GFX_DEBUG_PIXMAPS | GFX_DEBUG_BASIC, */
	sdl_set_parameter,
	sdl_init_specific,
	sdl_init,
	sdl_exit,
	sdl_draw_line,
	sdl_draw_filled_rect,
	sdl_register_pixmap,
	sdl_unregister_pixmap,
	sdl_draw_pixmap,
	sdl_grab_pixmap,
	sdl_update,
	sdl_set_static_buffer,
	sdl_set_pointer,
	sdl_set_palette,
	sdl_get_event,
	sdl_usec_sleep,
	NULL
};

#endif /* HAVE_SDL */
