/***************************************************************************
 sdl_driver.c Copyright (C) 2001 Solomon Peachy

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

/* set optimisations for Win32: */
/* g on: enable global optimizations */
/* t on: use fast code */
/* y on: suppress creation of frame pointers on stack */
/* s off: disable minimize size code */
#ifdef _WIN32
#	include <memory.h>
#	ifndef SATISFY_PURIFY
#		pragma optimize( "s", off )
#		pragma optimize( "gty", on )
#		pragma intrinsic( memcpy, memset )
#	endif
#endif

#include <sci_memory.h>

#include <gfx_driver.h>
#ifdef HAVE_SDL
#include <gfx_tools.h>

#ifndef _MSC_VER
#  include <sys/time.h>
#  include <SDL/SDL.h>
#else
#  include <SDL.h>
#  include <sci_win32.h>
#endif

#ifndef SDL_DISABLE
#	define SDL_DISABLE 0
#endif
#ifndef SDL_ALPHA_OPAQUE
#	define SDL_ALPHA_OPAQUE 255
#endif

#define SCI_SDL_HANDLE_NORMAL 0
#define SCI_SDL_HANDLE_GRABBED 1

#define SCI_SDL_SWAP_CTRL_CAPS (1 << 0)
#define SCI_SDL_FULLSCREEN (1 << 2)

#ifdef ARM_WINCE
#  include "../../wince/resource.h"	
#  include "../../wince/SDL_rotozoom.c"
#  define WIN32
#  include <SDL/SDL_syswm.h>
#  undef WIN32
#  define SHFS_SHOWTASKBAR            0x0001
#  define SHFS_HIDETASKBAR            0x0002
#  define SHFS_SHOWSIPBUTTON          0x0004
#  define SHFS_HIDESIPBUTTON          0x0008
#  define SHFS_SHOWSTARTICON          0x0010
#  define SHFS_HIDESTARTICON          0x0020
#  define SHA_INPUTDIALOG	      0x00000001

   LRESULT CALLBACK WindowProc(HWND hwnd, UINT nMsg, WPARAM wParam, LPARAM lParam);
   WNDPROC OldWindowProc;
   HBITMAP Banner;
   static LPTSTR szAppName = TEXT("FreeSCI");
   static LPTSTR AboutText = TEXT("FreeSCI for PocketPC, version "VERSION"\n\nPorted to PocketPC/ARM by\nIsmail Khatib");
   static HWND hwndMB = NULL;
   static SDL_Surface *toolbar1;
   static SDL_Surface *toolbar2;
   static SDL_Surface *toolbar3;
   static SDL_Surface **Toolbars[3]={&toolbar1,&toolbar2,&toolbar3};
   static SDL_Surface *pic_ctrl;
   static SDL_Surface *pic_alt;   
   static int CurrentToolbar;
   static int Modifier = 0;
   static int fullscreen = 0;

	
   typedef struct tagSHMENUBARINFO
   {
   	DWORD cbSize;               // IN  - Indicates which members of struct are valid
	HWND hwndParent;            // IN
   	DWORD dwFlags;              // IN  - Some features we want
   	UINT nToolBarId;            // IN  - Which toolbar are we using
   	HINSTANCE hInstRes;         // IN  - Instance that owns the resources
   	int nBmpId;
   	int cBmpImages;             // IN  - Count of bitmap images
	HWND hwndMB;                // OUT
    	COLORREF clrBk;             // IN  - background color of the menu bar (excluding sip)
   } SHMENUBARINFO, *PSHMENUBARINFO;
	
   typedef enum tagSIPSTATE
   {
   	SIP_UP = 0,
   	SIP_DOWN,
	SIP_FORCEDOWN,
    	SIP_UNCHANGED,
    	SIP_INPUTDIALOG,
   } SIPSTATE;
	
   typedef struct
   {
   	DWORD cbSize;
   	HWND hwndLastFocus;
   	UINT fSipUp :1;
   	UINT fSipOnDeactivation :1;
   	UINT fActive :1;
   	UINT fReserved :29;
   } SHACTIVATEINFO, *PSHACTIVATEINFO;
#endif /* ARM_WINCE */

static int
sdl_usec_sleep(struct _gfx_driver *drv, long usecs);

extern int string_truep(char *value);
static int flags = 0;

struct _sdl_state {
	int used_bytespp;
	gfx_pixmap_t *priority[2];
	SDL_Color colors[256];
	SDL_Surface *visual[3];
	SDL_Surface *primary;
#ifdef ARM_WINCE
	SDL_Surface *scl_buffer;
#endif
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
#define SDLERROR if ((debugline = __LINE__)) sdlprintf

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
		if (string_truep(value)){
			flags |= SCI_SDL_FULLSCREEN;
#ifdef ARM_WINCE			
			fullscreen = 1;
#endif			
		}
		else
			flags &= ~SCI_SDL_FULLSCREEN;

		return GFX_OK;
	}

	SDLERROR("Attempt to set sdl parameter \"%s\" to \"%s\"\n", attribute, value);
	return GFX_ERROR;
}


static unsigned char _sdl_zero = 0;
static void
sdl_set_null_pointer(struct _gfx_driver *drv)
{
	SDL_FreeCursor(SDL_GetCursor());
	SDL_SetCursor(SDL_CreateCursor(&_sdl_zero, &_sdl_zero, 1, 1, 0, 0));
	SDL_ShowCursor(SDL_ENABLE);
}

static int
sdl_init_specific(struct _gfx_driver *drv, int xfact, int yfact, int bytespp)
{
	int red_shift, green_shift, blue_shift, alpha_shift;
	int xsize = xfact * 320;
	int ysize = yfact * 200;

	int i;

#ifdef _MSC_VER /* Win32 doesn't support mouse pointers greater than 64x64 */
	if (xfact > 2 || yfact > 2)
		drv->capabilities &= ~GFX_CAPABILITY_MOUSE_POINTER;
#endif
#ifdef ARM_WINCE /* WinCE doesn't support mouse pointers (at least PocketPC) */
	drv->capabilities &= ~GFX_CAPABILITY_MOUSE_POINTER;
#endif
#ifdef __BEOS__ /* BeOS has been reported not to work well with the mouse pointer at all */
	drv->capabilities &= ~GFX_CAPABILITY_MOUSE_POINTER;
#endif

	if (!S)
		S = sci_malloc(sizeof(struct _sdl_state));
	if (!S)
		return GFX_FATAL;

	if (xfact < 1 || yfact < 1 || bytespp < 1 || bytespp > 4) {
		SDLERROR("Internal error: Attempt to open window w/ scale factors (%d,%d) and bpp=%d!\n",
		 xfact, yfact, bytespp);
	}

	S->primary = NULL;

#ifdef ARM_WINCE
	i = SDL_HWSURFACE | SDL_SWSURFACE | SDL_HWPALETTE | SDL_DOUBLEBUF | SDL_NOFRAME;
#endif
#ifdef _WIN32	
	i = SDL_HWSURFACE | SDL_SWSURFACE | SDL_HWPALETTE;
#else	
	i = SDL_HWSURFACE | SDL_SWSURFACE | SDL_HWPALETTE | SDL_DOUBLEBUF;
#endif
		if (flags & SCI_SDL_FULLSCREEN) {
		i |= SDL_FULLSCREEN;
	}

	/* If we are using mouse pointer emulation:
	** Set a transparent 1x1 pixel pointer so that SDL believes
	** that there actually is something to draw  */
	if (!drv->capabilities & GFX_CAPABILITY_MOUSE_POINTER)
		sdl_set_null_pointer(drv);

#ifdef ARM_WINCE
	if (!(fullscreen == 1)) {
		S->primary = SDL_SetVideoMode(240, 150, bytespp << 3, i);
	} else {
		S->primary = SDL_SetVideoMode(320, 240, bytespp << 3, i);
	}
#else
	S->primary = SDL_SetVideoMode(xsize, ysize, bytespp << 3, i);
#endif



	if (!S->primary) {
		SDLERROR("Could not set up a primary SDL surface!\n");
		return GFX_FATAL;
	}

	if (S->primary->format->BytesPerPixel != bytespp) {
		SDLERROR("Could not set up a primary SDL surface of depth %d bpp!\n",bytespp);
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
	SDL_EventState(SDL_VIDEORESIZE, SDL_IGNORE);
	SDL_EventState(SDL_KEYUP, SDL_IGNORE);
	if (drv->capabilities & GFX_CAPABILITY_MOUSE_POINTER)
		SDL_EventState(SDL_MOUSEMOTION, SDL_IGNORE);

	SDL_WM_SetCaption("FreeSCI", "freesci");
	
#ifdef ARM_WINCE
	if (!(flags & SCI_SDL_FULLSCREEN)) {
		/* get HWND from SDL-Window, stored in WMInfo.window */
		SDL_SysWMinfo WMInfo;
		SDL_VERSION(&WMInfo.version);
		SDL_GetWMInfo(&WMInfo);
		
		/* create menubar */
		WNDCLASS wc;
		wc.style = CS_HREDRAW | CS_VREDRAW;
		wc.lpfnWndProc = WindowProc;
		wc.cbClsExtra = 0;
		wc.cbWndExtra = 0;
		wc.hInstance = GetModuleHandle(NULL);
		wc.hIcon = (HICON)LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_FREESCI));
		wc.hCursor = NULL;
		wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
		wc.lpszMenuName = NULL;
		wc.lpszClassName = szAppName;
		RegisterClass(&wc);
		
		OldWindowProc = (WNDPROC) SetWindowLong(WMInfo.window, GWL_WNDPROC, (LONG) WindowProc); 
        	
		SHMENUBARINFO smbi;
		smbi.cbSize = sizeof(smbi); 
		smbi.hwndParent = WMInfo.window; 
		smbi.dwFlags = 0; 
		smbi.nToolBarId = IDM_MENU; 
		smbi.hInstRes = GetModuleHandle(NULL); 
		smbi.nBmpId = 0; 
		smbi.cBmpImages = 0; 
		smbi.hwndMB = NULL;
		BOOL res = SHCreateMenuBar(&smbi);
		hwndMB = smbi.hwndMB;
		
		
		MoveWindow(WMInfo.window, 0, 0, 240, 320, TRUE);
		
		SetWindowPos(WMInfo.window, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE|SWP_NOSIZE);
		SetForegroundWindow(WMInfo.window);
		SHFullScreen(WMInfo.window, SHFS_SHOWSIPBUTTON);
		SHFullScreen(WMInfo.window, SHFS_HIDETASKBAR);
		
		Banner = LoadBitmap(GetModuleHandle(NULL), MAKEINTRESOURCE(IDB_BANNER));
	}
#endif	
	SDL_ShowCursor(SDL_ENABLE);
	S->pointer_data[0] = NULL;
	S->pointer_data[1] = NULL;

	S->buckystate = 0;

	if (bytespp == 1) {
		red_shift = green_shift = blue_shift = alpha_shift = 0;
	} else {
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
			SDLERROR("Out of memory: Could not allocate priority maps! (%dx%d)\n",
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
			SDLERROR("Could not set up visual buffers!\n");
			return GFX_FATAL;
		}

		if (ALPHASURFACE)
		 SDL_SetAlpha(S->visual[i],SDL_SRCALPHA,SDL_ALPHA_OPAQUE);

		if (SDL_FillRect(S->primary, NULL, SDL_MapRGB(S->primary->format, 0,0,0)))
			SDLERROR("Couldn't fill backbuffer!\n");
	}
#ifdef ARM_WINCE
	if (!(flags & SCI_SDL_FULLSCREEN)) {
		/* create the scaling buffer */
		S->scl_buffer = NULL;
		S->scl_buffer = SDL_CreateRGBSurface(SDL_SRCALPHA,
					/* SDL_HWSURFACE | SDL_SWSURFACE, */
					xsize, ysize,
					bytespp << 3,
					S->primary->format->Rmask,
					S->primary->format->Gmask,
					S->primary->format->Bmask,
					S->alpha_mask);
		if (S->scl_buffer == NULL) {
			SDLERROR("Could not set up scaling buffer!\n");
			return GFX_FATAL;
		}
        	
		printf("Using scaling buffer SDL surface of %d,%d @%d bpp (%04x)\n",
			xsize, ysize, bytespp << 3, S->scl_buffer);
			
		if (SDL_FillRect(S->scl_buffer, NULL, SDL_MapRGB(S->primary->format, 0,0,0)))
				SDLERROR("Couldn't fill scaled backbuffer!\n");
	}
#endif /* (ARM_WINCE)  */
			
	SDL_EnableKeyRepeat(SDL_DEFAULT_REPEAT_DELAY, SDL_DEFAULT_REPEAT_INTERVAL);

	drv->mode = gfx_new_mode(xfact, yfact, bytespp,
			   S->primary->format->Rmask,
			   S->primary->format->Gmask,
			   S->primary->format->Bmask,
			   S->alpha_mask,
			   red_shift, green_shift, blue_shift, alpha_shift,
			   (bytespp == 1)? 256 : 0, 0); /*GFX_MODE_FLAG_REVERSE_ALPHA);*/
#ifdef ARM_WINCE
	if (flags & SCI_SDL_FULLSCREEN) {
		if (!toolbar1){		// if not already loaded...
			InitCE();
			UpdateCE(drv);
		}
	}
#endif				   
			   
	return GFX_OK;
}

#ifdef ARM_WINCE
/* Subclass of the SDL-Window */
LRESULT CALLBACK WindowProc(HWND hwnd, UINT nMsg, WPARAM wParam, LPARAM lParam)
{

	HDC		hdc, hdcBanner;
	HBITMAP		BanBuf;
	PAINTSTRUCT	ps;
	COLORREF	crTxt, crBk;
	static		SHACTIVATEINFO sai;
	switch (nMsg)
	{
	case WM_CREATE:
		memset(&sai, 0, sizeof(sai));
		SHSipPreference(hwnd, SIP_INPUTDIALOG);
		break;
	case WM_PAINT:
	        hdc = BeginPaint(hwnd, &ps);
	        hdcBanner = CreateCompatibleDC(NULL);
	        BanBuf = SelectObject(hdcBanner, Banner);

	        BitBlt(hdc, 0, 150, 240, 170, hdcBanner, 0, 0, SRCCOPY);
	        
	        SelectObject(hdcBanner, BanBuf);
	        DeleteDC(hdcBanner);
	        
		EndPaint (hwnd, &ps);
		SHSipPreference(hwnd, SIP_UP); /* Hack! */
		break;
	case WM_ACTIVATE:
		SHFullScreen(hwnd, SHFS_SHOWSIPBUTTON);
		SHHandleWMActivate(hwnd, wParam, lParam, &sai, SHA_INPUTDIALOG);
		break;
	case WM_SETTINGCHANGE:
		SHHandleWMSettingChange(hwnd, wParam, lParam, &sai);
		break;
	case WM_COMMAND:
		switch(wParam)
		{
		case IDC_ABOUT:
			MessageBox(hwnd, AboutText, szAppName, MB_OK | MB_ICONINFORMATION | MB_APPLMODAL); 
			break;
		case IDC_EXIT:
			sdl_exit();
			break;
		}
		break;
	}
	return CallWindowProc(OldWindowProc, hwnd, nMsg, wParam, lParam);
}		

SDL_Surface *LoadToolbar(char* TB)
{
	SDL_Surface *converted;
	SDL_Surface *sprite;
	sprite = SDL_LoadBMP(TB);
	converted = SDL_DisplayFormat(sprite);
	SDL_FreeSurface(sprite);
	return converted;
}

void InitCE()
{
	SDL_Rect r;

	if(toolbar1)
		SDL_FreeSurface(toolbar1);
	toolbar1=LoadToolbar("toolbar.bmp");
	if(toolbar2)
		SDL_FreeSurface(toolbar2);
	toolbar2=LoadToolbar("toolbar1.bmp");
	if(toolbar3)
		SDL_FreeSurface(toolbar3);
	toolbar3=LoadToolbar("toolbar2.bmp");
	
	if(pic_ctrl)
		SDL_FreeSurface(pic_ctrl);
	pic_ctrl = SDL_CreateRGBSurface(SDL_SWSURFACE, 17, 12, 16,
				toolbar1->format->Rmask,
				toolbar1->format->Gmask,
				toolbar1->format->Bmask,
				NULL);
						
	r.x=320; r.y=0;
	r.w=17; r.h=12;
	SDL_BlitSurface(toolbar1, &r, pic_ctrl, NULL);
	
	if(pic_alt)
		SDL_FreeSurface(pic_alt);
	pic_alt = SDL_CreateRGBSurface(SDL_SWSURFACE, 15, 12, 16,
				toolbar1->format->Rmask,
				toolbar1->format->Gmask,
				toolbar1->format->Bmask,
				NULL);
						
	r.x=320; r.y=12;
	r.w=15; r.h=12;
	SDL_BlitSurface(toolbar1, &r, pic_alt, NULL);
}
#endif /* (ARM_WINCE) */

static int
sdl_init(struct _gfx_driver *drv)
{
	int depth = 0;
	int i;

	if (SDL_Init(SDL_INIT_VIDEO|SDL_INIT_NOPARACHUTE)) {
		DEBUGB("Failed to init SDL\n");
		return GFX_FATAL;
	}
	SDL_EnableUNICODE(SDL_ENABLE);

#ifdef _WIN32
	i = SDL_HWSURFACE | SDL_SWSURFACE | SDL_HWPALETTE;
#else
	i = SDL_HWSURFACE | SDL_SWSURFACE | SDL_HWPALETTE | SDL_DOUBLEBUF;
#endif
	if (flags & SCI_SDL_FULLSCREEN) {
		i |= SDL_FULLSCREEN;
	}

	depth = SDL_VideoModeOK(XFACT*320,YFACT*200, 32, i);
	if (depth && (! sdl_init_specific(drv, XFACT, YFACT, depth >> 3 )))
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
#ifdef ARM_WINCE
		if (!(flags & SCI_SDL_FULLSCREEN)) {
			SDL_FreeSurface(S->scl_buffer);
			S->scl_buffer = NULL;
		
			DeleteObject(Banner);
		}
#endif
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
static void lineColor2(SDL_Surface *dst, Sint16 x1, Sint16 y1, Sint16 x2, Sint16 y2, Uint32 color)
{
	int pixx, pixy;
	int x,y;
	int dx,dy;
	int sx,sy;
	int swaptmp;
	Uint8 *pixel;

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
	int xfact = (line_mode == GFX_LINE_MODE_FINE)? 1: XFACT;
	int yfact = (line_mode == GFX_LINE_MODE_FINE)? 1: YFACT;
	int xsize = S->visual[1]->w;
	int ysize = S->visual[1]->h;

	if (color.mask & GFX_MASK_VISUAL) {
		int xc, yc;
		rect_t newline;

		scolor = sdl_map_color(drv, color);
		newline.xl = line.x;
		newline.yl = line.y;

		for (xc = 0; xc < xfact; xc++)
			for (yc = 0; yc < yfact; yc++) {
				newline.x = line.x + xc;
				newline.y = line.y + yc;
				newline.xl = line.x + line.xl + xc;
				newline.yl = line.y + line.yl + yc;

				if (newline.x < 0)
					newline.x = 0;
				if (newline.xl < 0)
					newline.xl = 0;
				if (newline.y < 0)
					newline.y = 0;
				if (newline.yl < 0)
					newline.yl = 0;
				if (newline.x > xsize)
					newline.x = xsize;
				if (newline.xl >= xsize)
					newline.xl = xsize -1;
				if (newline.y > ysize)
					newline.y = ysize;
				if (newline.yl >= ysize)
					newline.yl = ysize -1;

#if 0
				fprintf(stderr, "draw %d %d %d %d %08x %d %d\n", newline.x,
					newline.y, newline.xl, newline.yl, scolor, xsize, ysize);
#endif

				lineColor2(S->visual[1], (Sint16)newline.x, (Sint16)newline.y,
					  (Sint16)newline.xl, (Sint16)newline.yl, scolor);
			}
	}

	if (color.mask & GFX_MASK_PRIORITY) {
		int xc, yc;
		rect_t newline;

		newline.xl = line.xl;
		newline.yl = line.yl;

		for (xc = 0; xc < xfact; xc++)
			for (yc = 0; yc < yfact; yc++) {
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
			SDLERROR("Can't fill rect");
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
		SDLERROR("Attempt to register pixmap twice!\n");
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
		SDLERROR("Attempt to unregister pixmap twice!\n");
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
		SDLERROR("Attempt to scale pixmap (%dx%d)->(%dx%d): Not supported\n",
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
			SDLERROR("blt failed");
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
		SDLERROR("Failed to allocate SDL surface");
		return GFX_ERROR;
	}

	srect.x = dest.x;
	srect.y = dest.y;
	drect.x = 0;
	drect.y = 0;

	if(SDL_BlitSurface(S->visual[bufnr], &srect, temp, &drect))
		SDLERROR("blt failed");

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
		SDLERROR("blt failed");

	SDL_FreeSurface(temp);
	return GFX_OK;
}

static int
sdl_grab_pixmap(struct _gfx_driver *drv, rect_t src, gfx_pixmap_t *pxm,
		gfx_map_mask_t map)
{


	if (src.x < 0 || src.y < 0) {
		SDLERROR("Attempt to grab pixmap from invalid coordinates (%d,%d)\n", src.x, src.y);
		return GFX_ERROR;
	}

	if (!pxm->data) {
		SDLERROR("Attempt to grab pixmap to unallocated memory\n");
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
			SDLERROR("Failed to allocate SDL surface");
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
			SDLERROR("grab_pixmap:  grab blit failed!\n");

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
		SDLERROR("FIXME: priority map grab not implemented yet!\n");
		break;

	default:
		SDLERROR("Attempt to grab pixmap from invalid map 0x%02x\n", map);
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
#ifdef ARM_WINCE
	SDL_Surface *temp;
#endif

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
			SDLERROR("surface update failed!\n");

		if ((src.x == dest.x) && (src.y == dest.y))
			gfx_copy_pixmap_box_i(S->priority[0], S->priority[1], src);
		break;
	case GFX_BUFFER_FRONT:
	
#ifdef ARM_WINCE
		if (flags & SCI_SDL_FULLSCREEN) {
			if (SDL_BlitSurface(S->visual[data_source], &srect, S->primary, &drect))
				SDLERROR("primary surface update failed!\n");
			UpdateCE(drv);				
		} else {
			if (SDL_BlitSurface(S->visual[data_source], &srect, S->scl_buffer, &drect))
				SDLERROR("scaled buffer surface update failed!\n");
			
			temp = zoomSurface(S->scl_buffer, 0.75, 0.75, 1);		

			if (SDL_BlitSurface(temp, NULL, S->primary, NULL))
				SDLERROR("primary surface update failed!\n");
				
			SDL_FreeSurface(temp);				
		}

#else
		if (SDL_BlitSurface(S->visual[data_source], &srect, S->primary, &drect))
			SDLERROR("primary surface update failed!\n");			
#endif			
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
		SDLERROR("Attempt to set static buffer with unregisterd pixmap!\n");
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
	byte *data = sci_calloc(linewidth, lines);
	byte *linebase = data, *pos;
	byte *src = pointer->index_data;

	for (yc = 0; yc < pointer->index_yl; yc++) {
		int scalectr;
		int bitc = 7;
		pos = linebase;

		for (xc = 0; xc < pointer->index_xl; xc++) {
			int draw = mode ? (*src == 0) : (*src < 255);
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
#ifdef ARM_WINCE					// Hardware cursor button 'rotate' down ->left
	case SDLK_DOWN:
		if (flags & SCI_SDL_FULLSCREEN) {
			return SCI_K_LEFT;
		} else {
			return SCI_K_DOWN;
		}
#else
	case SDLK_DOWN: return SCI_K_DOWN;
#endif	
	case SDLK_KP3:
	case SDLK_PAGEDOWN: return SCI_K_PGDOWN;
	case SDLK_KP4:
#ifdef ARM_WINCE					// Hardware cursor button 'rotate' left -> up
	case SDLK_LEFT:
		if (flags & SCI_SDL_FULLSCREEN) {
			return SCI_K_UP;
		} else {
			return SCI_K_LEFT;
		}
#else
	case SDLK_LEFT: return SCI_K_LEFT;
#endif	
	case SDLK_KP5: return SCI_K_CENTER;
	case SDLK_KP6:
#ifdef ARM_WINCE					// Hardware cursor button 'rotate' right -> down
	case SDLK_RIGHT:
		if (flags & SCI_SDL_FULLSCREEN) {
			return SCI_K_DOWN;
		} else {
			return SCI_K_RIGHT;
		}
#else
	case SDLK_RIGHT: return SCI_K_RIGHT;
#endif	
	case SDLK_KP7:
	case SDLK_HOME: return SCI_K_HOME;
	case SDLK_KP8:
#ifdef ARM_WINCE					// Hardware cursor button 'rotate' up -> right
	case SDLK_UP:
		if (flags & SCI_SDL_FULLSCREEN) {
			return SCI_K_RIGHT;
		} else {
			return SCI_K_UP;
		}
#else
	case SDLK_UP: return SCI_K_UP;
#endif	
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
	case SDLK_LMETA:
	case SDLK_RMETA:
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
	case SDLK_GREATER: return rkey;
	case SDLK_SPACE: return ' ';

	case SDLK_BACKQUOTE:
		if (keysym.mod & KMOD_CTRL)
			return '`';
		else
			return rkey;
	}

	sciprintf("Unknown SDL keysym: %04x (%d) \n", skey, rkey);
	return 0;
}

#ifdef ARM_WINCE
void UpdateCE(gfx_driver_t *drv)
{
	SDL_Rect r,r2;
	r.x=0; r.y=200;
	r.w=320; r.h=40;
	r2.x=0; r2.y=0;
	r2.w=320; r2.h=40;
	SDL_BlitSurface(*(Toolbars[CurrentToolbar]), &r2, S->primary, &r);
	switch (Modifier){
		case KMOD_CTRL:
			r2.x=1; r2.y=214;
			r2.w=pic_ctrl->w; r2.h=pic_ctrl->h;			
			SDL_BlitSurface(pic_ctrl, NULL, S->primary, &r2);
			break;
		case KMOD_ALT:
			r2.x=19; r2.y=227;		
			r2.w=pic_alt->w; r2.h=pic_alt->h;
			SDL_BlitSurface(pic_alt, NULL, S->primary, &r2);
			break;			
		default:
			break;
	}
}

void HandleToolbarCE(gfx_driver_t *drv, SDL_Event event, sci_event_t *sci_event)
{
#define	SCI_K_NOTHING 255
	int L=(event.button.y-200)/13;
	int B=(event.button.x-10)/13;
	int MapTB;
	
	if (L==0){	// clicked in Line 1
		if (event.button.x >= 202){	// clicked in num-part
			B=(event.button.x-10)/13;
		} else {
			B=(event.button.x-10)/13;
		}
	}
	else if (L==1){	// clicked in Line 2
		if (event.button.x >= 202){	// clicked in num-part
			B=(event.button.x-10)/13;
		} else {
			B=(event.button.x-13)/13;
		}
	}
	else if (L==2){	// clicked in Line 3
		if (event.button.x >= 202){	// clicked in num-part	
			B=(event.button.x-10)/13;
		} else {
			B=(event.button.x-11)/13;
		}
	}

	static struct KeyInfo
	{
		int code;
		int ascii;
	} TB [2][3][24]={
	  { {
		{SCI_K_ESC,27},
		{9,9},			// TAB
		{'q','q'},			
		{'w','w'},		
		{'e','e'},		
		{'r','r'},		
		{'t','t'},		
		{'y','y'},		
		{'u','u'},		
		{'i','i'},		
		{'o','o'},
		{'p','p'},
		{'[','['},
		{']',']'},
		{SCI_K_BACKSPACE,8},
		{'1','1'},
		{'2','2'},
		{'3','3'},
		{'4','4'},
		{'5','5'},
		{'-','-'},
		{SCI_K_HOME,71},
		{SCI_K_UP,72},
		{SCI_K_PGUP,73}
	    }, 
	    {
	    	{KMOD_CTRL,-1},
		{SCI_K_NOTHING,-5},	// SWITCH TO FN-TOOLBAR	    	
		{'a','a'},
		{'s','s'},
		{'d','d'},	
		{'f','f'},
		{'g','g'},
		{'h','h'},	
		{'j','j'},
		{'k','k'},
		{'l','l'},	
		{';',';'},
		{'\'','\''},
		{'`','`'},		
		{SCI_K_ENTER,13},
		{'6','6'},
		{'7','7'},
		{'8','8'},
		{'9','9'},
		{'0','0'},
		{'=','='},
		{SCI_K_LEFT,75},
		{SCI_K_NOTHING,-2},
		{SCI_K_RIGHT,77}
	    },
	    { 
		{KMOD_CAPS,-1},	// -1 means "lock"
		{KMOD_ALT,-1},
		{'\\','\\'},
		{'z','z'},
		{'x','x'},
		{'c','c'},
		{'v','v'},		
		{'b','b'},
		{'n','n'},
		{'m','m'},
		{',',','},	
		{'.','.'},
		{'/','/'},
		{SCI_K_ENTER,13},
		{SCI_K_ENTER,13},
		{' ',' '},
		{' ',' '},
		{' ',' '},
		{' ',' '},
		{' ',' '},
		{' ',' '},
		{SCI_K_END,79},		
		{SCI_K_DOWN,80},
		{SCI_K_PGDOWN,81}
	    },
	  },
	  { {
	  	{SCI_K_ESC,27},
		{SCI_K_NOTHING,-2},
		{SCI_K_F1,59},
		{SCI_K_F2,60},
		{SCI_K_F3,61},
		{SCI_K_F4,62},
		{SCI_K_F5,63},
		{SCI_K_F6,64},
		{SCI_K_F7,65},	
		{SCI_K_F8,66},	
		{SCI_K_F9,67},	
		{SCI_K_F10,68},	
		{SCI_K_NOTHING,-2},	
		{SCI_K_NOTHING,-2},	
		{SCI_K_BACKSPACE,8},	
		{SCI_K_NOTHING,-2},	
		{SCI_K_INSERT,82},
		{SCI_K_HOME,71},
		{SCI_K_PGUP,73},
		{SCI_K_NOTHING,-2},	
		{SCI_K_NOTHING,-2},	
		{SCI_K_HOME,71},
		{SCI_K_UP,72},
		{SCI_K_PGUP,73}
	    },
	    {						
	    	{KMOD_CTRL,-1},
		{SCI_K_NOTHING,-5},	// SWITCH TOOLBAR	    	
		{SCI_K_NOTHING,-2},	
		{SCI_K_NOTHING,-2},	
		{SCI_K_NOTHING,-2},	
		{SCI_K_NOTHING,-2},	
		{SCI_K_NOTHING,-2},	
		{SCI_K_NOTHING,-2},	
		{SCI_K_NOTHING,-2},	
		{SCI_K_NOTHING,-2},	
		{SCI_K_NOTHING,-2},	
		{SCI_K_NOTHING,-2},	
		{SCI_K_NOTHING,-2},	
		{SCI_K_NOTHING,-2},
		{SCI_K_ENTER,13},
		{SCI_K_NOTHING,-2},		
		{SCI_K_DELETE,83},
		{SCI_K_END,79},
		{SCI_K_PGDOWN,81},
		{SCI_K_NOTHING,-2},
		{SCI_K_NOTHING,-2},
		{SCI_K_LEFT,75},
		{SCI_K_NOTHING,-2},
		{SCI_K_RIGHT,77}
	    },
	    {	
		{SCI_K_NOTHING,-2},
		{KMOD_ALT,-1},
		{SCI_K_NOTHING,-2},				
		{SCI_K_NOTHING,-2},				
		{SCI_K_NOTHING,-2},				
		{SCI_K_NOTHING,-2},				
		{SCI_K_NOTHING,-2},				
		{SCI_K_NOTHING,-2},				
		{SCI_K_NOTHING,-2},				
		{SCI_K_NOTHING,-2},				
		{SCI_K_NOTHING,-2},				
		{SCI_K_NOTHING,-2},				
		{SCI_K_NOTHING,-2},				
		{SCI_K_ENTER,13},
		{SCI_K_ENTER,13},
		{' ',' '},
		{' ',' '},
		{' ',' '},
		{' ',' '},
		{' ',' '},
		{' ',' '},
		{SCI_K_END,79},		
		{SCI_K_DOWN,80},
		{SCI_K_PGDOWN,81}
	    }
	  }
	}; 

	if(B<0 || B>23 || L<0 || L>2)
		return;
		
	MapTB = CurrentToolbar;
	if (MapTB==2) MapTB=0;		// Shifted 'abc' mapped to non shifted

	int code=TB[MapTB][L][B].code;
	int ascii=TB[MapTB][L][B].ascii;

	switch(ascii)
	{
	    case -1:
			//sciprintf("lockkey!\n");					    
			break;
	    case -2:
			// DO NOTHING
			break;			
	    case -6:
			//CPU_CycleIncrease();
			break;
	    case -7:
			//CPU_CycleDecrease();
	    		break;
	    case -8:
			//IncreaseFrameSkip();
			break;
	    case -9:
			//DecreaseFrameSkip();
	    		break;
	    case -4:
			//sdl_exit(); // Quick Exit (not clean yet)
			break;
	    case -5:
	    		CurrentToolbar = (CurrentToolbar+1)%2;
	    		S->buckystate =  0;
	    		sci_event->buckybits = S->buckystate;
			UpdateCE(drv);
			SDL_Flip(S->primary);
	    		break;
	    case 0:
	    		if(!code)
		    		CurrentToolbar = (CurrentToolbar+1)%2;
				UpdateCE(drv);
				SDL_Flip(S->primary);
	    		break;
	};
		
	if(code!=SCI_K_NOTHING && code) // && ascii>0)
	{
		if(ascii==-1)
		{	
                	switch (code) {
                	case KMOD_CAPS:
                		if (!(S->buckystate ==  (SCI_EVM_LSHIFT | SCI_EVM_RSHIFT))) {
                			CurrentToolbar = 2;
                			S->buckystate =  SCI_EVM_LSHIFT | SCI_EVM_RSHIFT;
                			Modifier = KMOD_CAPS;
					UpdateCE(drv);
					SDL_Flip(S->primary);                			
                		} else {
                			CurrentToolbar = 0;
                			S->buckystate =  0;
                			Modifier = 0;
					UpdateCE(drv);
					SDL_Flip(S->primary);                			
                		}
                		break;
                	case KMOD_CTRL:
                		if (!(S->buckystate ==  SCI_EVM_CTRL)) {
                			S->buckystate =  SCI_EVM_CTRL;
                			Modifier = KMOD_CTRL;
					UpdateCE(drv);
					SDL_Flip(S->primary);                			
                		} else {
                			S->buckystate =  0;
                			Modifier = 0;
					UpdateCE(drv);
					SDL_Flip(S->primary);                			
                		}
                		break;
                	case KMOD_ALT:
                		if (!(S->buckystate ==  SCI_EVM_ALT)) {
                			S->buckystate =  SCI_EVM_ALT;
                			Modifier = KMOD_ALT;
					UpdateCE(drv);
					SDL_Flip(S->primary);                			
                		} else {
                			S->buckystate =  0;
                			Modifier = 0;
					UpdateCE(drv);
					SDL_Flip(S->primary);                			
                		}
                		break;
                	case KMOD_NUM:
                		if (!(S->buckystate ==  SCI_EVM_NUMLOCK)) {
                			S->buckystate =  SCI_EVM_NUMLOCK;
                			Modifier = KMOD_NUM;
                		} else {
                			S->buckystate =  0;
                			Modifier = 0;
                		}
                		break;
                	case KMOD_RSHIFT:
                		if (!(S->buckystate ==  SCI_EVM_RSHIFT)) {
                			S->buckystate =  SCI_EVM_RSHIFT;
                			Modifier = KMOD_RSHIFT;
                		} else {
                			S->buckystate =  0;
                			Modifier = 0;
                		}
                		break;			
                	case KMOD_LSHIFT:
                		if (!(S->buckystate ==  SCI_EVM_LSHIFT)) {
                			S->buckystate =  SCI_EVM_LSHIFT;
                			Modifier = KMOD_LSHIFT;
                		} else {
                			S->buckystate =  0;
                			Modifier = 0;
                		}
                		break;																				
                	default:
                		S->buckystate = 0;
                		Modifier = 0;
                		break;
                	}		
                	sci_event->buckybits = S->buckystate;
		}
		else if (ascii>0)		
		{
			sci_event->type = SCI_EVT_KEYBOARD;
			sci_event->buckybits = S->buckystate;			
			sci_event->data = code;
		}
	}
}
#endif

void
sdl_fetch_event(gfx_driver_t *drv, long wait_usec, sci_event_t *sci_event)
{
	SDL_Event event;
	int x_button_xlate[] = {0, 1, 3, 2, 4, 5};
	struct timeval ctime, timeout_time, sleep_time;
	long usecs_to_sleep;
	long time_sec, time_usec;

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
				break;
			}
			case SDL_MOUSEBUTTONDOWN:
#ifdef ARM_WINCE
				if (flags & SCI_SDL_FULLSCREEN) {
					if(event.button.y>200)
						{
						HandleToolbarCE(drv,event, sci_event);
						return;
					}
				}
#endif
				sci_event->type = SCI_EVT_MOUSE_PRESS;
				sci_event->buckybits = S->buckystate;
				sci_event->data = event.button.button - 1;
#ifdef ARM_WINCE
				if (!(flags & SCI_SDL_FULLSCREEN)) {
					drv->pointer_x = event.button.x *4/3;
					drv->pointer_y = event.button.y *4/3;
				} else {
					drv->pointer_x = event.button.x;
					drv->pointer_y = event.button.y;
				}
#else				
				drv->pointer_x = event.button.x;
				drv->pointer_y = event.button.y;
#endif				
				return;
			case SDL_MOUSEBUTTONUP:
#ifdef ARM_WINCE
				if (flags & SCI_SDL_FULLSCREEN) {
					if(event.button.y>200)
					return;
				}
#endif			
				sci_event->type = SCI_EVT_MOUSE_RELEASE;
				sci_event->buckybits = S->buckystate;
				sci_event->data = event.button.button - 1;
#ifdef ARM_WINCE
				if (!(flags & SCI_SDL_FULLSCREEN)) {
					drv->pointer_x = event.button.x *4/3;
					drv->pointer_y = event.button.y *4/3;
				} else {
					drv->pointer_x = event.button.x;
					drv->pointer_y = event.button.y;
				}
#else				
				drv->pointer_x = event.button.x;
				drv->pointer_y = event.button.y;
#endif
				return;
			case SDL_MOUSEMOTION:
#ifdef ARM_WINCE
				if (flags & SCI_SDL_FULLSCREEN) {
					if(event.motion.y>200)
						return;
				}
#endif						
#ifdef ARM_WINCE
				if (!(flags & SCI_SDL_FULLSCREEN)) {
					drv->pointer_x = event.motion.x *4/3;
					drv->pointer_y = event.motion.y *4/3;				
				} else {
					drv->pointer_x = event.motion.x;
					drv->pointer_y = event.motion.y;					
				}
#else				
				drv->pointer_x = event.motion.x;
				drv->pointer_y = event.motion.y;
#endif
				break;
			case SDL_QUIT:
				sci_event->type = SCI_EVT_QUIT;
				return;
				break;
			case SDL_VIDEOEXPOSE:
				break;
			default:
				SDLERROR("Received unhandled SDL event %04x\n", event.type);
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

			sdl_usec_sleep(drv, usecs_to_sleep);
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
sdl_usec_sleep(struct _gfx_driver *drv, long usecs)
{
	struct timeval ctime;
	if (usecs > 10000)
		usecs = 10000;

	ctime.tv_sec = 0;
	ctime.tv_usec = usecs;

#ifdef _MSC_VER
	sci_sched_yield(); /* usleep on win32 doesn't really sleep, so let's give up the rest of the quantum to play nice with the sound thread */
#elif ARM_WINCE 
	sci_sched_yield(); /* see above :) */
#else
	usleep(usecs);  /* let's try this out instead, no? */
	/*  select(0, NULL, NULL, NULL, &ctime); /* Sleep. */
#endif

	return GFX_OK;
}

gfx_driver_t
gfx_driver_sdl = {
	"sdl",
	"0.3",
	SCI_GFX_DRIVER_MAGIC,
	SCI_GFX_DRIVER_VERSION,
	NULL,
	0, 0,
	GFX_CAPABILITY_MOUSE_SUPPORT | GFX_CAPABILITY_MOUSE_POINTER
	| GFX_CAPABILITY_PIXMAP_REGISTRY | GFX_CAPABILITY_PIXMAP_GRABBING
	| GFX_CAPABILITY_FINE_LINES,
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


/* reset to original optimisations for Win32: */
/* (does not reset intrinsics) */
#ifdef _WIN32
//#pragma optimize( "", on )
#endif

