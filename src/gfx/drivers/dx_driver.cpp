/***************************************************************************
 dx_driver.cpp Copyright (C) 2002 Alexander R Angas,
               Copyright (C) 1999 Dmitry Jemerov

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

		Alexander R Angas (Alex) <wgd@internode.on.net>

 History:

   20020817 - DirectX 8.0a (Win95+) support.
   20030113 - Submitted to CVS.
   20030115 - Fixed: memory leak in dx_grab_pixmap()
			- Fixed: vertical line drawing
   20030116 - Fixed: all known memory leaks
			- TODO: implement size factor
			- TODO: implement mouse
			- TODO: improve LockRect call for draw_filled_rect
			        calling draw_line
            - TODO: allow setting of adapter
                -- Alex Angas

***************************************************************************/

#ifdef HAVE_DIRECTX

#ifndef __cplusplus
#error NOTE: This file MUST be compiled as C++. In Visual C++, use the /Tp command line option.
#endif

//#define _WIN32_WINNT 0x0400	// For TrackMouseEvent

#include <windows.h>
#include <d3d8.h>
#include <d3dx8math.h>
#include <dxerr8.h>

#if (DIRECT3D_VERSION < 0x0800)
#error ERROR: DirectX 8.0a or higher is required for this driver.
#endif

extern "C" {
#include <gfx_system.h>
#include <gfx_driver.h>
#include <gfx_tools.h>
#include <assert.h>
#include <uinput.h>
#include <ctype.h>
#include <console.h> // for sciprintf
#include <sci_win32.h>
#include <sci_memory.h>
};


#define DODX(cmd, proc)        \
		hr = cmd;              \
		if ( hr != D3D_OK ) {  \
			sciprintf("%s(): Failure in %i\n  %s\n  %s\n", #proc, __LINE__, #cmd, DXGetErrorString8(hr));  \
			BREAKPOINT();      \
		}

#define SAFE_RELEASE(p)  \
	if (p)               \
		(p)->Release();

#define dx_state ((struct gfx_dx_struct_t *)(drv->state))

#define MAP_KEY(x,y) case x: add_key_event ((struct gfx_dx_struct_t *)(drv->state), y); break


#define DX_CLASS_NAME "FreeSCI DirectX Graphics"
#define WINDOW_SUFFIX " Window"


struct PIXMAP_VERTEX
{
	D3DXVECTOR4 p;
	DWORD col;			// Diffuse colour
	D3DXVECTOR2 t;		// 2D texture coordinates
};
#define D3DFVF_PIXMAP_VERTEX (D3DFVF_XYZRHW | D3DFVF_DIFFUSE | D3DFVF_TEX1)


#define SCI_DX_HANDLE_NORMAL 0
#define SCI_DX_HANDLE_GRABBED 1

#define NUM_VISUAL_BUFFERS		3
#define NUM_PRIORITY_BUFFERS	2

#define PRIMARY_VIS	0
#define BACK_VIS	1
#define STATIC_VIS	2

#define BACK_PRI	0
#define STATIC_PRI	1

struct gfx_dx_struct_t
{
	LPDIRECT3D8 g_pD3D;				// D3D object
	D3DCAPS8 g_d3dcaps;				// Capabilities of device
	D3DDISPLAYMODE g_d3ddm;			// Width and height of screen
	D3DPRESENT_PARAMETERS g_d3dpp;	// Presentation parameters
	LPDIRECT3DDEVICE8 g_pd3dDevice; // Rendering device

	LPDIRECT3DVERTEXBUFFER8 g_pPVB;	// Buffer to hold pixmap vertices
	//UINT pvNum = 4;						// Count of pixmap vertices
	PIXMAP_VERTEX pvData[4];		// Buffer of pixmap vertex structs

	LPDIRECT3DTEXTURE8 visuals[NUM_VISUAL_BUFFERS];
	LPDIRECT3DTEXTURE8 priority_visuals[NUM_PRIORITY_BUFFERS];
	gfx_pixmap_t *priority_maps[NUM_PRIORITY_BUFFERS];

	WNDCLASSEX wc;		// Window class
	HWND hWnd;			// Window
	UINT xsize, ysize;
	UINT xfact, yfact;
	UINT bpp;

	// event queue
	int queue_size, queue_first, queue_last;
	sci_event_t *event_queue;
};

static int ProcessMessages(struct _gfx_driver *drv);


/* Properly draws the scene. */
static gfx_return_value_t
Render2D(struct _gfx_driver *drv)
{
	HRESULT hr;

    // Render the pixmaps
	DODX((dx_state->g_pd3dDevice->SetTexture( 0, dx_state->visuals[PRIMARY_VIS] )), Render2D);
	DODX((dx_state->g_pd3dDevice->SetTextureStageState( 0, D3DTSS_COLOROP,   D3DTOP_MODULATE )), Render2D);
	DODX((dx_state->g_pd3dDevice->SetTextureStageState( 0, D3DTSS_COLORARG1, D3DTA_TEXTURE )), Render2D);
	DODX((dx_state->g_pd3dDevice->SetTextureStageState( 0, D3DTSS_COLORARG2, D3DTA_DIFFUSE )), Render2D);
	DODX((dx_state->g_pd3dDevice->SetTextureStageState( 0, D3DTSS_ALPHAOP,   D3DTOP_DISABLE )), Render2D);

	// Assume vertex buffer has already been set up
	DODX((dx_state->g_pd3dDevice->SetStreamSource( 0, dx_state->g_pPVB, sizeof(PIXMAP_VERTEX) )), Render2D);
	DODX((dx_state->g_pd3dDevice->SetVertexShader( D3DFVF_PIXMAP_VERTEX )), Render2D);
	DODX((dx_state->g_pd3dDevice->DrawPrimitive( D3DPT_TRIANGLESTRIP, 0, 2 )), Render2D);

	return GFX_OK;
}

/* Abstractly draws the scene. */
static gfx_return_value_t
Render(struct _gfx_driver *drv)
{
	HRESULT hr;

	// Do nothing if no D3D device
	if( dx_state == NULL )
		return GFX_OK;
	if( dx_state->g_pd3dDevice == NULL )
		return GFX_OK;

	// Clear the back buffer
	DODX((dx_state->g_pd3dDevice->Clear( 0, NULL, D3DCLEAR_TARGET, D3DCOLOR_XRGB(0,0,0), 1.0f, 0 )), Render);

	// Begin the scene
	DODX((dx_state->g_pd3dDevice->BeginScene()), Render);

	// Render
	Render2D(drv);

	// End the scene
	DODX((dx_state->g_pd3dDevice->EndScene()), Render);

	// Present the backbuffer contents to the display
	DODX((dx_state->g_pd3dDevice->Present( NULL, NULL, NULL, NULL )), Render);

	return GFX_OK;
}


static gfx_return_value_t
InitD3D(struct _gfx_driver *drv)
{
	HRESULT hr;

	sciprintf("Setting up DirectX Graphics...\n");

	// Create Direct3D object.
	dx_state->g_pD3D = Direct3DCreate8( D3D_SDK_VERSION );
	if ( FAILED( dx_state->g_pD3D ) ) {
		sciprintf("InitD3D(): Direct3DCreate8 failed\n");
		return GFX_FATAL;
	}

	// Look for adapters.
	for ( UINT adapterLoop = 0; adapterLoop < dx_state->g_pD3D->GetAdapterCount(); adapterLoop++ )
	{
		D3DADAPTER_IDENTIFIER8 adapterId;
		DODX((dx_state->g_pD3D->GetAdapterIdentifier(adapterLoop, D3DENUM_NO_WHQL_LEVEL, &adapterId)), InitD3D);
		if ( FAILED( hr ) )
			break;
		if (adapterId.Driver[0] == '\0')
			break;
		sciprintf(" Adapter %u: %s\n", adapterLoop++, adapterId.Description);
	}

	// Get device caps.
	DODX((dx_state->g_pD3D->GetDeviceCaps(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, &(dx_state->g_d3dcaps))), InitD3D);
	if (dx_state->g_d3dcaps.Caps2 & D3DCAPS2_CANRENDERWINDOWED)
		sciprintf("   Can render to window or full screen\n");
	else
		sciprintf("   Can render to full screen only\n");
	if (dx_state->g_d3dcaps.DevCaps & D3DDEVCAPS_DRAWPRIMTLVERTEX)
		sciprintf("   DrawPrimitive aware\n");
	else
		sciprintf("   NOT DrawPrimitive aware\n");
	if (dx_state->g_d3dcaps.DevCaps &= D3DPTEXTURECAPS_SQUAREONLY)
		sciprintf("   Square textures ONLY\n");
	else
		sciprintf("   Non-square textures allowed\n");

	// Get current display mode.
	DODX((dx_state->g_pD3D->GetAdapterDisplayMode( D3DADAPTER_DEFAULT, &dx_state->g_d3ddm )), InitD3D);
	sciprintf("   Display %lu x %lu\n", dx_state->g_d3ddm.Width, dx_state->g_d3ddm.Height);

	// Set D3D behaviour.
	ZeroMemory( &dx_state->g_d3dpp, sizeof(D3DPRESENT_PARAMETERS) );
	dx_state->g_d3dpp.BackBufferWidth  = dx_state->xsize;
	dx_state->g_d3dpp.BackBufferHeight = dx_state->ysize;
	dx_state->g_d3dpp.BackBufferFormat = dx_state->g_d3ddm.Format;
	dx_state->g_d3dpp.BackBufferCount = 1;
	dx_state->g_d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
	dx_state->g_d3dpp.hDeviceWindow = dx_state->hWnd;
	dx_state->g_d3dpp.Windowed = TRUE;

	// Create D3D device.
	DODX((dx_state->g_pD3D->CreateDevice(
		D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, dx_state->hWnd,
		D3DCREATE_SOFTWARE_VERTEXPROCESSING,	// TODO: try changing this?
		&dx_state->g_d3dpp, &dx_state->g_pd3dDevice)), InitD3D );

    // Set render states.
	DODX((dx_state->g_pd3dDevice->SetRenderState( D3DRS_ZENABLE, D3DZB_FALSE )), InitD3D);
	DODX((dx_state->g_pd3dDevice->SetRenderState( D3DRS_SHADEMODE, D3DSHADE_FLAT )), InitD3D);
	DODX((dx_state->g_pd3dDevice->SetRenderState( D3DRS_CULLMODE, D3DCULL_NONE )), InitD3D);
	DODX((dx_state->g_pd3dDevice->SetRenderState( D3DRS_LIGHTING, FALSE )), InitD3D);
	DODX((dx_state->g_pd3dDevice->SetRenderState( D3DRS_AMBIENT, RGB(255,255,255) )), InitD3D);

	// Create textures.
	int i;
	for (i = 0; i < NUM_VISUAL_BUFFERS; i++) {
		DODX((dx_state->g_pd3dDevice->CreateTexture(dx_state->xsize, dx_state->ysize,
			1, 0,
			D3DFMT_A8R8G8B8,
			D3DPOOL_MANAGED,
			&dx_state->visuals[i])), InitD3D);
	}
	for (i = 0; i < NUM_PRIORITY_BUFFERS; i++) {
		DODX((dx_state->g_pd3dDevice->CreateTexture(dx_state->xsize, dx_state->ysize,
			1, 0,
			D3DFMT_P8,
			D3DPOOL_MANAGED,
			&dx_state->priority_visuals[i])), InitD3D);
	}

	// Set up pixmap vertex buffers.
	DODX((dx_state->g_pd3dDevice->CreateVertexBuffer( 4 * sizeof(PIXMAP_VERTEX),
												0, D3DFVF_PIXMAP_VERTEX,
												D3DPOOL_MANAGED,
												&dx_state->g_pPVB )), InitD3D);

	dx_state->pvData[0].p = D3DXVECTOR4(                  0.0f,                   0.0f, 0.0f, 1.0f);
    dx_state->pvData[1].p = D3DXVECTOR4((float)dx_state->xsize,                   0.0f, 0.0f, 1.0f);
    dx_state->pvData[2].p = D3DXVECTOR4(                  0.0f, (float)dx_state->ysize, 0.0f, 1.0f);
    dx_state->pvData[3].p = D3DXVECTOR4((float)dx_state->xsize, (float)dx_state->ysize, 0.0f, 1.0f);
	dx_state->pvData[0].col = dx_state->pvData[1].col = dx_state->pvData[2].col = dx_state->pvData[3].col = 0xffffffff;
	dx_state->pvData[0].t = D3DXVECTOR2(0.0f, 0.0f);
	dx_state->pvData[1].t = D3DXVECTOR2(1.0f, 0.0f);
	dx_state->pvData[2].t = D3DXVECTOR2(0.0f, 1.0f);
	dx_state->pvData[3].t = D3DXVECTOR2(1.0f, 1.0f);

	VOID *ptr;
	DODX((dx_state->g_pPVB->Lock(0, 0, (BYTE**)&ptr, 0)), InitD3D);
	memcpy(ptr, dx_state->pvData, sizeof(dx_state->pvData));
	DODX((dx_state->g_pPVB->Unlock()), InitD3D);

	return GFX_OK;

}


LRESULT WINAPI MsgProc( HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam )
{
	return DefWindowProc( hWnd, msg, wParam, lParam );
}


/* Sets a driver-specific parameter. */
static int
dx_set_param(struct _gfx_driver *drv, char *attribute, char *value)
{
	//sciprintf("dx_set_param(): '%s' to '%s'\n", attribute, value);
	return GFX_ERROR;
}


/* Attempts to initialize a specific graphics mode. */
static int
dx_init_specific(struct _gfx_driver *drv,
				 int xfact, int yfact,	/* horizontal and vertical scaling */
				 int bytespp)			/* any of GFX_COLOR_MODE_* */
{
	int red_shift = 8, green_shift = 16, blue_shift = 24, alpha_shift = 32;
	int alpha_mask = 0x00000000, red_mask = 0x00ff0000, green_mask = 0x0000ff00, blue_mask = 0x000000ff;
	gfx_return_value_t d3dret;

	drv->state = (struct gfx_dx_struct_t *) sci_malloc(sizeof(gfx_dx_struct_t));

	dx_state->xfact = xfact;
	dx_state->yfact = yfact;
	dx_state->xsize = xfact * 320;
	dx_state->ysize = yfact * 200;
	bytespp = 4;
	dx_state->bpp = bytespp;

	// Register the window class.
	ZeroMemory( &(dx_state->wc), sizeof(WNDCLASSEX) );
	dx_state->wc.cbSize = sizeof(WNDCLASSEX);
	dx_state->wc.style = CS_HREDRAW | CS_VREDRAW;
	dx_state->wc.lpfnWndProc = MsgProc;
	dx_state->wc.hInstance = GetModuleHandle(NULL);
	dx_state->wc.lpszClassName = DX_CLASS_NAME;
	if ( RegisterClassEx( &dx_state->wc ) == 0 )
	{
		sciprintf("dx_init_specific(): RegisterClassEx failed (%u)\n", GetLastError());
		return GFX_FATAL;
	}

	// Create the application's window.
	dx_state->hWnd = CreateWindow(
		DX_CLASS_NAME, "FreeSCI",
		WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT, CW_USEDEFAULT, dx_state->xsize, dx_state->ysize,
		GetDesktopWindow(), NULL, dx_state->wc.hInstance, NULL );

	if ( dx_state->hWnd == NULL )
	{
		sciprintf("dx_init_specific(): CreateWindow failed (%u)\n", GetLastError());
		return GFX_FATAL;
	}

	for (int i = 0; i < NUM_PRIORITY_BUFFERS; i++) {
		dx_state->priority_maps[i] = gfx_pixmap_alloc_index_data(gfx_new_pixmap(dx_state->xsize, dx_state->ysize, GFX_RESID_NONE, -i, -777));
		if (!dx_state->priority_maps[i]) {
			GFXERROR("Out of memory: Could not allocate priority maps! (%dx%d)\n", dx_state->xsize, dx_state->ysize);
			return GFX_FATAL;
		}
		dx_state->priority_maps[i]->flags |= GFX_PIXMAP_FLAG_SCALED_INDEX;
	}

	// Initialise Direct3D stuff.
	d3dret = InitD3D(drv);
	if (d3dret != GFX_OK)
		return d3dret;

	// Set up mouse tracking
#if(_WIN32_WINNT >= 0x0400)
	TRACKMOUSEEVENT trkMouse;
	ZeroMemory(&trkMouse, sizeof(TRACKMOUSEEVENT));
	trkMouse.cbSize = sizeof(TRACKMOUSEEVENT);
	trkMouse.dwFlags |= TME_LEAVE;
	trkMouse.hwndTrack = dx_state->hWnd;
	if ( TrackMouseEvent(&trkMouse) == NULL )
	{
		sciprintf("dx_init_specific(): TrackMouseEvent failed (%u)\n", GetLastError());
	}
#endif

	// Show the window
	ShowWindow( dx_state->hWnd, SW_SHOWDEFAULT );
	UpdateWindow( dx_state->hWnd );

	// Set up the driver's state
	dx_state->queue_first = 0;
	dx_state->queue_last  = 0;
	dx_state->queue_size  = 256;
	dx_state->event_queue = (sci_event_t *) sci_malloc (dx_state->queue_size * sizeof (sci_event_t));

	// Set up graphics mode
	drv->mode = gfx_new_mode(xfact, yfact, bytespp,
			   red_mask, green_mask, blue_mask, alpha_mask,
			   red_shift, green_shift, blue_shift, alpha_shift,
			   (bytespp == 1) ? 256 : 0, 0);

	return GFX_OK;
}


/* Initialize 'most natural' graphics mode. */
static int
dx_init(struct _gfx_driver *drv)
{
	// TODO: optimise later
	return dx_init_specific(drv, 1, 1, 1);
}


/* Uninitializes the current graphics mode. */
static void
dx_exit(struct _gfx_driver *drv)
{
	int i;

	if(drv->state == NULL)
		return;

	for (i = 0; i < NUM_PRIORITY_BUFFERS; i++)
		SAFE_RELEASE( dx_state->priority_visuals[i] );
	for (i = 0; i < NUM_VISUAL_BUFFERS; i++)
		SAFE_RELEASE( dx_state->visuals[i] );
	SAFE_RELEASE( dx_state->g_pPVB );
    SAFE_RELEASE( dx_state->g_pd3dDevice );
    SAFE_RELEASE( dx_state->g_pD3D );

	if ( dx_state->event_queue )
		sci_free(dx_state->event_queue);
	dx_state->queue_size = 0;

	for (i = 0; i < NUM_PRIORITY_BUFFERS; i++)
		gfx_free_pixmap(drv, dx_state->priority_maps[i]);

    UnregisterClass( DX_CLASS_NAME WINDOW_SUFFIX, dx_state->wc.hInstance );
	DestroyWindow(dx_state->hWnd);

	sci_free(dx_state);
}


/*** Drawing operations. ***/

/* Draws a single line to the back buffer. */
static int
dx_draw_line(struct _gfx_driver *drv, rect_t line, gfx_color_t color,
		    gfx_line_mode_t line_mode, gfx_line_style_t line_style)
{
	HRESULT hr;
	D3DLOCKED_RECT lockedRect;

	if (color.mask & GFX_MASK_VISUAL) {
		// Calculate line size
		int xf = (line_mode == GFX_LINE_MODE_FINE) ? 1 : dx_state->xfact;
		int yf = (line_mode == GFX_LINE_MODE_FINE) ? 1 : dx_state->yfact;

		// Calculate colour value for line pixel
		UINT lineColor = (color.alpha << 24) | (color.visual.r << 16) | (color.visual.g << 8) | color.visual.b;
		RECT r = { line.x, line.y, line.x + line.xl + xf, line.y + line.yl + yf };

		/*** HACK ***/
		if (r.left == r.right)
			r.right++;
		if (r.top == r.bottom)
			r.bottom++;
		if (r.right > dx_state->xsize)
			r.right = dx_state->xsize;
		if (r.bottom > dx_state->ysize)
			r.bottom = dx_state->ysize;
//sciprintf("%08X  %i,%i -> %i,%i\n", lineColor, r.left, r.top, r.right, r.bottom);

		DODX( (dx_state->visuals[BACK_VIS]->LockRect(0, &lockedRect, &r, 0)), dx_draw_line );
		UINT *rectPtr = (UINT*)lockedRect.pBits;

		// Going along x axis
		for (int y_pixel = r.top; y_pixel < (r.bottom * yf); y_pixel++)
		{
			UINT *startLine = rectPtr;
			for (int x_pixel = r.left; x_pixel < (r.right * xf); x_pixel++)
			{
				*rectPtr = lineColor;
				rectPtr++;
			}
			rectPtr = startLine;
			rectPtr += dx_state->xsize;
		}

		DODX( (dx_state->visuals[BACK_VIS]->UnlockRect(0)), dx_draw_line );
	}

	if (color.mask & GFX_MASK_PRIORITY) {
		unsigned int xc, yc;
		rect_t newline;

		newline.xl = line.xl;
		newline.yl = line.yl;

		for (xc = 0; xc < dx_state->xfact; xc++)
			for (yc = 0; yc < dx_state->yfact; yc++) {
				newline.x = line.x + xc;
				newline.y = line.y + yc;
				gfx_draw_line_pixmap_i(dx_state->priority_maps[BACK_PRI], newline, color.priority);
			}
	}

	return GFX_OK;
}


/* Draws a single filled and possibly shaded rectangle to the back buffer. */
static int
dx_draw_filled_rect(struct _gfx_driver *drv, rect_t box,
			   gfx_color_t color1, gfx_color_t color2,
			   gfx_rectangle_fill_t shade_mode)
{
	RECT rcDest;
	byte *pri;

	rcDest.left   = box.x;
	rcDest.top    = box.y;
	rcDest.right  = box.x+box.xl;
	rcDest.bottom = box.y+box.yl;

	if (color1.mask & GFX_MASK_VISUAL)
	{
		dx_draw_line(drv, box, color1, GFX_LINE_MODE_FAST, GFX_LINE_STYLE_NORMAL);
	}

	if (color1.mask & GFX_MASK_PRIORITY)
	{
		pri = dx_state->priority_maps[BACK_PRI]->index_data + box.x + box.y*(dx_state->xsize);
		for(int i=0; i<box.yl; i++)
		{
			memset(pri,color1.priority,box.xl);
			pri += dx_state->xsize;
		}
	}

	return GFX_OK;
}


/*** Pixmap operations. ***/

static int
dx_register_pixmap(struct _gfx_driver *drv, gfx_pixmap_t *pxm)
{
	HRESULT hr;

	int i, xs;
	byte *s, *d;
	D3DLOCKED_RECT lockedRect;
	LPDIRECT3DTEXTURE8 newTex;
	DODX( (dx_state->g_pd3dDevice->CreateTexture(pxm->xl, pxm->yl, 1, 0, D3DFMT_A8R8G8B8, D3DPOOL_MANAGED, &newTex )), dx_register_pixmap );

	// Do gfx crossblit
	DODX( (newTex->LockRect(0, &lockedRect, NULL, 0)), dx_register_pixmap );
	s = pxm->data;
	d = (byte *) lockedRect.pBits;
	xs = drv->mode->bytespp * pxm->xl;

	for(i = 0; i < pxm->yl; i++)
	{
		memcpy(d, s, xs);
		s += xs;
		d += lockedRect.Pitch;
	}
	DODX( (newTex->UnlockRect(0)), dx_register_pixmap );

	pxm->internal.info = newTex;
	pxm->internal.handle = SCI_DX_HANDLE_NORMAL;

	return GFX_OK;
}


static int
dx_unregister_pixmap(struct _gfx_driver *drv, gfx_pixmap_t *pxm)
{
	SAFE_RELEASE((LPDIRECT3DTEXTURE8) (pxm->internal.info));
	pxm->internal.info = NULL;

	return GFX_OK;
}


/* Draws part of a pixmap to the static or back buffer. */
static int
dx_draw_pixmap(struct _gfx_driver *drv, gfx_pixmap_t *pxm, int priority,
		      rect_t src, rect_t dest, gfx_buffer_t buffer)
{
	HRESULT hr;
	int bufnr = (buffer == GFX_BUFFER_STATIC) ? 2 : 1;
	int pribufnr = bufnr - 1;
	LPDIRECT3DTEXTURE8 srct, dstt;
	LPDIRECT3DSURFACE8 sbuf, dbuf;
	D3DLOCKED_RECT lockedRect;
	byte *pri_map = NULL;

	if (pxm->internal.handle == SCI_DX_HANDLE_GRABBED) {
		// copy from pxm->internal.info to visual[bufnr]
		RECT srcRect = RECT_T_TO_RECT(src);
		POINT dstPoint = { dest.x, dest.y };

		srct = (LPDIRECT3DTEXTURE8) (pxm->internal.info);
		dstt = dx_state->visuals[bufnr];

		DODX( (srct->GetSurfaceLevel(0, &sbuf)), dx_draw_pixmap );
		DODX( (dstt->GetSurfaceLevel(0, &dbuf)), dx_draw_pixmap );
		DODX( (dx_state->g_pd3dDevice->CopyRects(sbuf, &srcRect, 1, dbuf, &dstPoint)), dx_draw_pixmap );

		SAFE_RELEASE(sbuf);
		SAFE_RELEASE(dbuf);

		return GFX_OK;
	}


	// Create texture to temporarily hold visuals[bufnr]
	LPDIRECT3DTEXTURE8 temp;
	DODX( (dx_state->g_pd3dDevice->CreateTexture(pxm->xl, pxm->yl, 1, 0, D3DFMT_A8R8G8B8, D3DPOOL_MANAGED, &temp)), dx_draw_pixmap );
	RECT srcRect = RECT_T_TO_RECT(dest);
	RECT destRect = { 0, 0, dest.xl, dest.yl };
	POINT dstPoint = { destRect.left, destRect.top };

	// Copy from visuals[bufnr] to temp
	srct = dx_state->visuals[bufnr];
	dstt = temp;
	DODX( (srct->GetSurfaceLevel(0, &sbuf)), dx_draw_pixmap );
	DODX( (dstt->GetSurfaceLevel(0, &dbuf)), dx_draw_pixmap );
	DODX( (dx_state->g_pd3dDevice->CopyRects(sbuf, &srcRect, 1, dbuf, &dstPoint)), dx_draw_pixmap );

	// Copy from given pixmap to temp
	DODX( (dbuf->LockRect(&lockedRect, &destRect, 0)), dx_draw_pixmap );
	gfx_crossblit_pixmap(drv->mode, pxm, priority, src, dest,
		       (byte *) lockedRect.pBits, lockedRect.Pitch,
		       dx_state->priority_maps[pribufnr]->index_data,
		       dx_state->priority_maps[pribufnr]->index_xl, 1,
		       GFX_CROSSBLIT_FLAG_DATA_IS_HOMED);
	DODX( (dbuf->UnlockRect()), dx_draw_pixmap );

	SAFE_RELEASE(sbuf);
	SAFE_RELEASE(dbuf);


	// Copy from temp to visuals[bufnr]
	RECT src2Rect = { 0, 0, dest.xl, dest.yl };
	POINT dst2Point = { dest.x, dest.y };

	srct = temp;
	dstt = dx_state->visuals[bufnr];
	DODX( (srct->GetSurfaceLevel(0, &sbuf)), dx_draw_pixmap );
	DODX( (dstt->GetSurfaceLevel(0, &dbuf)), dx_draw_pixmap );
	DODX( (dx_state->g_pd3dDevice->CopyRects(sbuf, &src2Rect, 1, dbuf, &dst2Point)), dx_draw_pixmap );

	SAFE_RELEASE(sbuf);
	SAFE_RELEASE(dbuf);
	SAFE_RELEASE(temp);

	return GFX_OK;
}


/* Grabs an image from the visual or priority back buffer. */
static int
dx_grab_pixmap(struct _gfx_driver *drv, rect_t src, gfx_pixmap_t *pxm,
		      gfx_map_mask_t map)
{
	HRESULT hr;

	if (src.x < 0 || src.y < 0) {
		GFXERROR("Attempt to grab pixmap from invalid coordinates (%d,%d)\n", src.x, src.y);
		return GFX_ERROR;
	}

	if (!pxm->data) {
		GFXERROR("Attempt to grab pixmap to unallocated memory\n");
		return GFX_ERROR;
	}

	// Choose map to grab from
	switch (map) {

	case GFX_MASK_VISUAL: {
		LPDIRECT3DTEXTURE8 temp;
		LPDIRECT3DSURFACE8 tempSrf, backSrf;
		CONST RECT srcRect = RECT_T_TO_RECT(src);
		CONST POINT dstPoint = { 0, 0 };

		pxm->xl = src.xl;
		pxm->yl = src.yl;

		DODX( (dx_state->g_pd3dDevice->CreateTexture(pxm->xl, pxm->yl, 1, 0, D3DFMT_A8R8G8B8, D3DPOOL_MANAGED, &temp)), dx_grab_pixmap );

		DODX( (dx_state->visuals[BACK_VIS]->GetSurfaceLevel(0, &backSrf)), dx_grab_pixmap );
		DODX( (temp->GetSurfaceLevel(0, &tempSrf)), dx_grab_pixmap );
		DODX( (dx_state->g_pd3dDevice->CopyRects(backSrf, &srcRect, 1, tempSrf, &dstPoint)), dx_grab_pixmap );

		pxm->internal.info = temp;
		pxm->internal.handle = SCI_DX_HANDLE_GRABBED;
		pxm->flags |= GFX_PIXMAP_FLAG_INSTALLED | GFX_PIXMAP_FLAG_EXTERNAL_PALETTE | GFX_PIXMAP_FLAG_PALETTE_SET;

		SAFE_RELEASE(backSrf);
		SAFE_RELEASE(tempSrf);

		break;
	}

	case GFX_MASK_PRIORITY:
		sciprintf("FIXME: priority map grab not implemented yet!\n");
		break;

	default:
		sciprintf("Attempt to grab pixmap from invalid map 0x%02x\n", map);
		return GFX_ERROR;
	}

	return GFX_OK;
}


/*** Buffer operations ***/

/* Updates the front buffer or the back buffers. */
static int
dx_update(struct _gfx_driver *drv, rect_t src, point_t dest, gfx_buffer_t buffer)
{
	HRESULT hr;
	LPDIRECT3DTEXTURE8 srct, dstt;
	LPDIRECT3DSURFACE8 sbuf, dbuf;
	CONST RECT srcRect = RECT_T_TO_RECT(src);
	CONST POINT dstPoint = { dest.x, dest.y };

	switch (buffer) {

	case GFX_BUFFER_FRONT:
		srct = dx_state->visuals[BACK_VIS];
		dstt = dx_state->visuals[PRIMARY_VIS];

		DODX( (srct->GetSurfaceLevel(0, &sbuf)), dx_update );
		DODX( (dstt->GetSurfaceLevel(0, &dbuf)), dx_update );

		DODX( (dx_state->g_pd3dDevice->CopyRects(sbuf, &srcRect, 1, dbuf, &dstPoint)), dx_update );

		SAFE_RELEASE(sbuf);
		SAFE_RELEASE(dbuf);

		Render(drv);
		break;

	case GFX_BUFFER_BACK:
		if (src.x == dest.x && src.y == dest.y)
			gfx_copy_pixmap_box_i(dx_state->priority_maps[BACK_PRI], dx_state->priority_maps[STATIC_PRI], src);

		srct = dx_state->visuals[STATIC_VIS];
		dstt = dx_state->visuals[BACK_VIS];

		DODX( (srct->GetSurfaceLevel(0, &sbuf)), dx_update );
		DODX( (dstt->GetSurfaceLevel(0, &dbuf)), dx_update );

		DODX( (dx_state->g_pd3dDevice->CopyRects(sbuf, &srcRect, 1, dbuf, &dstPoint)), dx_update );

		SAFE_RELEASE(sbuf);
		SAFE_RELEASE(dbuf);

		break;

	default:
		GFXERROR("Invalid buffer %d in update!\n", buffer);
		return GFX_ERROR;
	}

	return GFX_OK;
}

/* Sets the contents of the static visual and priority buffers. */
static int
dx_set_static_buffer(struct _gfx_driver *drv, gfx_pixmap_t *pic,
			    gfx_pixmap_t *priority)
{
	if (!pic->internal.info) {
		GFXERROR("Attempt to set static buffer with unregistered pixmap!\n");
		return GFX_ERROR;
	}

	HRESULT hr;
	LPDIRECT3DTEXTURE8 pii = (LPDIRECT3DTEXTURE8) (pic->internal.info);
	LPDIRECT3DSURFACE8 pbf;
	LPDIRECT3DTEXTURE8 vis = dx_state->visuals[STATIC_VIS];
	LPDIRECT3DSURFACE8 vbf;

	// Copy from pic to visual[static]
	DODX( (pii->GetSurfaceLevel(0, &pbf)), dx_set_static_buffer );
	DODX( (vis->GetSurfaceLevel(0, &vbf)), dx_set_static_buffer );
	DODX( (dx_state->g_pd3dDevice->CopyRects(pbf, NULL, 0, vbf, NULL)), dx_set_static_buffer );

	SAFE_RELEASE(pbf);
	SAFE_RELEASE(vbf);

	// Copy priority map
	gfx_copy_pixmap_box_i(dx_state->priority_maps[STATIC_PRI], priority, gfx_rect(0, 0, dx_state->xsize, dx_state->ysize));

	return GFX_OK;
}


/*** Mouse pointer operations ***/

/* Sets a new mouse pointer. */
static int
dx_set_pointer(struct _gfx_driver *drv, gfx_pixmap_t *pointer)
{
	HRESULT hr;
	sciprintf("WARNING: dx_set_pointer() unimplemented\n");
	DODX( (dx_state->g_pd3dDevice->ShowCursor(FALSE)), dx_set_pointer );
/*
	LPDIRECT3DSURFACE8 pntSurf;
	D3DLOCKED_RECT lockedRect;

	rect_t src = { 0, 0, pointer->index_xl, pointer->index_yl };
	rect_t dst = { 0, 0, pointer->index_xl, pointer->index_yl };


	DODX( (dx_state->g_pd3dDevice->CreateImageSurface(pointer->index_xl, pointer->index_yl, D3DFMT_A8R8G8B8, &pntSurf)), dx_set_pointer );

	DODX( (pntSurf->LockRect(&lockedRect, NULL, 0)), dx_set_pointer );

	_gfx_crossblit_simple((byte*)lockedRect.pBits, pointer->index_data,
		lockedRect.Pitch, pointer->xl * dx_state->bpp,
		pointer->index_xl, pointer->index_yl, dx_state->bpp);

	DODX( (pntSurf->UnlockRect()), dx_set_pointer );

	DODX( (dx_state->g_pd3dDevice->SetCursorProperties(0, 0, pntSurf)), dx_set_pointer );
	DODX( (dx_state->g_pd3dDevice->ShowCursor(TRUE)), dx_set_pointer );
*/
	return GFX_OK;
}


/*** Event management ***/

static sci_event_t
get_queue_event(gfx_dx_struct_t *ctx)
{
	if (ctx->queue_first == ctx->queue_size)
		ctx->queue_first = 0;

	if (ctx->queue_first == ctx->queue_last)
	{
		sci_event_t noevent;
		noevent.data = 0;
		noevent.type = SCI_EVT_NONE;
		noevent.buckybits = 0;
		return noevent;
	}
	else
		return ctx->event_queue [ctx->queue_first++];
}

static void add_queue_event(gfx_dx_struct_t *ctx, int type, int data, short buckybits)
{
	if ((ctx->queue_last+1) % ctx->queue_size == ctx->queue_first)
	{
		/* Reallocate queue */
		int i, event_count;
		sci_event_t *new_queue;

		new_queue = (sci_event_t *) sci_malloc (ctx->queue_size * 2 * sizeof (sci_event_t));
		event_count = (ctx->queue_last - ctx->queue_first) % ctx->queue_size;
		for (i=0; i<event_count; i++)
			new_queue [i] = ctx->event_queue [(ctx->queue_first+i) % ctx->queue_size];
		free (ctx->event_queue);
		ctx->event_queue = new_queue;
		ctx->queue_size *= 2;
		ctx->queue_first = 0;
		ctx->queue_last  = event_count;
	}

	ctx->event_queue [ctx->queue_last].data = data;
	ctx->event_queue [ctx->queue_last].type = type;
	ctx->event_queue [ctx->queue_last++].buckybits = buckybits;
	if (ctx->queue_last == ctx->queue_size)
		ctx->queue_last=0;
}

/* Returns the next event in the event queue for this driver. */
static sci_event_t
dx_get_event(struct _gfx_driver *drv)
{
	assert(drv->state != NULL);
	return get_queue_event(dx_state);
}


/* Sleeps the specified amount of microseconds, or until the mouse moves. */
static int
dx_usleep(struct _gfx_driver *drv, long usecs)
{
	if (usecs < 1000)
	{
		sleep(0);
	} else {
		sleep(usecs/1000);
	}
	ProcessMessages(drv);
	return GFX_OK;
}

static void add_key_event (gfx_dx_struct_t *ctx, int data)
{
	short buckybits = 0;

	if (GetAsyncKeyState (VK_RSHIFT))
		buckybits |= SCI_EVM_RSHIFT;
	if (GetAsyncKeyState (VK_LSHIFT))
		buckybits |= SCI_EVM_LSHIFT;
	if (GetAsyncKeyState (VK_CONTROL))
		buckybits |= SCI_EVM_CTRL;
	if (GetAsyncKeyState (VK_MENU))
		buckybits |= SCI_EVM_ALT;
	if (GetKeyState (VK_SCROLL) & 1)
		buckybits |= SCI_EVM_SCRLOCK;
	if (GetKeyState (VK_NUMLOCK) & 1)
		buckybits |= SCI_EVM_NUMLOCK;
	if (GetKeyState (VK_CAPITAL) & 1)
		buckybits |= SCI_EVM_CAPSLOCK;

	add_queue_event (ctx, SCI_EVT_KEYBOARD, data, buckybits);
}


/*
static int
add_mouse_event(struct _gfx_driver *drv, int type, int data, LPARAM lParam, WPARAM wParam)
{
	drv->pointer_x = GET_X_LPARAM(lParam);
	drv->pointer_y = GET_Y_LPARAM(lParam);

//TODO:	add_queue_event (ctx,type, data, buckybits);
	return 0;
}
*/


/* Check for Windows messages. */
static int
ProcessMessages(struct _gfx_driver *drv)
{
	MSG msg;

	while (PeekMessage (&msg, NULL, 0, 0, PM_REMOVE)) {
		switch( msg.message )
		{
			case WM_PAINT:
				ValidateRect( dx_state->hWnd, NULL );
				Render(drv);
				break;

			case WM_KEYDOWN:
				switch (msg.wParam)
				{
					MAP_KEY (VK_ESCAPE,		SCI_K_ESC);
					MAP_KEY (VK_BACK,		SCI_K_BACKSPACE);
					MAP_KEY (VK_RETURN,		SCI_K_ENTER);
					MAP_KEY (VK_TAB,		SCI_K_TAB);
					MAP_KEY (VK_END,		SCI_K_END);
					MAP_KEY (VK_DOWN,		SCI_K_DOWN);
					MAP_KEY (VK_NEXT,		SCI_K_PGDOWN);
					MAP_KEY (VK_LEFT,		SCI_K_LEFT);
					MAP_KEY (VK_RIGHT,		SCI_K_RIGHT);
					MAP_KEY (VK_HOME,		SCI_K_HOME);
					MAP_KEY (VK_UP,			SCI_K_UP);
					MAP_KEY (VK_PRIOR,		SCI_K_PGUP);
					MAP_KEY (VK_INSERT,		SCI_K_INSERT);
					MAP_KEY (VK_DELETE,		SCI_K_DELETE);
					MAP_KEY (VK_DECIMAL,	SCI_K_DELETE);
					MAP_KEY (VK_ADD,		SCI_K_PLUS);
					MAP_KEY (VK_OEM_PLUS,	SCI_K_EQUALS);
					MAP_KEY (VK_SUBTRACT,	SCI_K_MINUS);
					MAP_KEY (VK_OEM_MINUS,	SCI_K_MINUS);
					MAP_KEY (VK_MULTIPLY,	SCI_K_MULTIPLY);
					MAP_KEY (VK_DIVIDE,		SCI_K_DIVIDE);
					MAP_KEY (VK_OEM_COMMA,	',');
					MAP_KEY (VK_OEM_PERIOD,	'.');
					MAP_KEY (VK_OEM_1,		';');	// US keyboards only
					MAP_KEY (VK_OEM_2,		'/');
					MAP_KEY (VK_OEM_3,		'`');
					MAP_KEY (VK_OEM_4,		'[');
					MAP_KEY (VK_OEM_5,		'\\');
					MAP_KEY (VK_OEM_6,		']');
					MAP_KEY (VK_OEM_7,		'\'');
					MAP_KEY (VK_F1,			SCI_K_F1);
					MAP_KEY (VK_F2,			SCI_K_F2);
					MAP_KEY (VK_F3,			SCI_K_F3);
					MAP_KEY (VK_F4,			SCI_K_F4);
					MAP_KEY (VK_F5,			SCI_K_F5);
					MAP_KEY (VK_F6,			SCI_K_F6);
					MAP_KEY (VK_F7,			SCI_K_F7);
					MAP_KEY (VK_F8,			SCI_K_F8);
					MAP_KEY (VK_F9,			SCI_K_F9);
					MAP_KEY (VK_F10,		SCI_K_F10);

					case VK_RSHIFT:
					case VK_LSHIFT:
					case VK_CONTROL:
					case VK_MENU:
					case VK_SCROLL:
					case VK_NUMLOCK:
					case VK_CAPITAL:
						break;	// ignore

				default:
					if (msg.wParam >= 'A' && msg.wParam <= 'Z')
						add_key_event (dx_state, msg.wParam - 'A' + 97);
					else if (msg.wParam >= VK_NUMPAD0 && msg.wParam <= VK_NUMPAD9)
					{
						if (GetKeyState (VK_NUMLOCK) & 1)
							add_key_event (dx_state, msg.wParam - VK_NUMPAD0 + '0');
						else
						switch (msg.wParam)
						{
							MAP_KEY (VK_NUMPAD0, SCI_K_INSERT);
							MAP_KEY (VK_NUMPAD1, SCI_K_END);
							MAP_KEY (VK_NUMPAD2, SCI_K_DOWN);
							MAP_KEY (VK_NUMPAD3, SCI_K_PGDOWN);
							MAP_KEY (VK_NUMPAD4, SCI_K_LEFT);
							MAP_KEY (VK_NUMPAD6, SCI_K_RIGHT);
							MAP_KEY (VK_NUMPAD7, SCI_K_HOME);
							MAP_KEY (VK_NUMPAD8, SCI_K_UP);
							MAP_KEY (VK_NUMPAD9, SCI_K_PGUP);
						}
					}
					else if (msg.wParam == 0xC0)  // tilde key - used for invoking console
						add_key_event (dx_state, '`');
					else
						add_key_event (dx_state, msg.wParam);
					break;
				}
				break;

#if(_WIN32_WINNT >= 0x0400)
			case WM_MOUSEMOVE:
				// Turn off mouse cursor
				ShowCursor(FALSE);
				break;

			case WM_MOUSELEAVE:
				// Turn on mouse cursor
				ShowCursor(TRUE);
				break;
#endif

			case WM_DESTROY:
				PostQuitMessage( 0 );
				drv->exit(drv);
				exit(-1);
				break;

		}
	}

	return 0;
}


extern "C"
gfx_driver_t gfx_driver_dx = {
	"directx",
	"0.1",
	SCI_GFX_DRIVER_MAGIC,
	SCI_GFX_DRIVER_VERSION,
	NULL,	/* mode */
	0, 0,	/* mouse pointer position */
	/*GFX_CAPABILITY_MOUSE_POINTER | GFX_CAPABILITY_COLOR_MOUSE_POINTER | */ GFX_CAPABILITY_PIXMAP_REGISTRY | GFX_CAPABILITY_PIXMAP_GRABBING | GFX_CAPABILITY_FINE_LINES | GFX_CAPABILITY_WINDOWED /*| GFX_CAPABILITY_MOUSE_SUPPORT*/,
	0,
	dx_set_param,
	dx_init_specific,
	dx_init,
	dx_exit,
	dx_draw_line,
	dx_draw_filled_rect,
	dx_register_pixmap,
	dx_unregister_pixmap,
	dx_draw_pixmap,
	dx_grab_pixmap,
	dx_update,
	dx_set_static_buffer,
	NULL,//dx_set_pointer,
	NULL,
	dx_get_event,
	dx_usleep,
	NULL
};

#endif
