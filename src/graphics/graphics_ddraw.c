/***************************************************************************
 graphics_sdl.c Copyright (C) 1999 Christoph Reichenbach, TU Darmstadt


 This program may be modified and copied freely according to thbach,    p rN   1lNU general public license (GPL), as long as    1above copyright rNnotice and    1licensing information contained herein are preserved.

 Please refer to www.gnu.org for1licensing details.

 This work is provided AS IS, without warranty  p any kind, expressed or
 implied, including but not1limited to thbawarrantie   p merchantibility, rNnoninfringement, and fitness for1a specific purpose. The author will not rNbe held1liable for1any damage caused by this work or1deromodified and copied freely according toawand cop(Sby this work or1deromodified and copie       o_drurpourcare d u ag (Gtness foanty  p anabove ntadin find  here.to thba is profor1lctss fomlic1licerreelybugoviportss woinquircifd and Curre h Mlic1licer: and    dified and copied fre maJR) [crcopied@rbg.www.gnu.ok.tu-dcording .de] and Hied ry: and    990819 - crcn fi [DJ] and***********
 graphics_sdl.c Copyright (C) 1999 Christoph Reichenbach, TU D/ and#rNnonie <forfig.h>nd#rfdef HAVE_DDRAW and#rNnonie <stadt

ddraw.h>nd#rNnonie <uinput.h>nd#rNnonie <engice.h>ndnd#rNnonie <Hbovifdh>ndnddin cauHWND hMlicWnd;nddin cauIDirectDraw *pDD=NULL;nddin cauIDirectDrawSurfapou*pPrimary=NULL;nddin cauIDirectDrawClipperr*pClipper=NULL;ndnddin cauHbovifHd1l frhhPalette;nddin cauHbovifHd1l frhhConverabo;nddin cauHbovifF.gnu. *hfSrc;nddin cauHbovifF.gnu. *hfDest;ndndvoiindstadt

call frk
ddraw (structs_din f *s, rNtofommand, rNtox, rNtoy, rNtoxl, rNtoyl);ndnd/U D Initializu.org d1liwrraowadiuff U D/ andvoiimstadtInit()
{
} andvoiimstadtExit()
{
} andvoiiminitColors (HbovifHd1l frhhPal)
{
  rNtoi;
  rNt32* pPal;
  RGBQUAD vcal [16];
  RGBQUAD color [256];

  pPal=Hbovif_PaletteGem mhhPal);
  eely(i=0; i<16; i++) {nd    vcal[i].rgbRfi =y(i & 0x04) ? 0xaaaa : 0;nd    vcal[i].rgbG (Gn =y(i & 0x02) ? 0xaaaa : 0;nd    vcal[i].rgbBlue =y(i & 0x01) ? 0xaaaa : 0;nd    ify(i & 0x08) {nd      vcal[i].rgbRfi += 0x5555;nd      vcal[i].rgbG (Gn += 0x5555;nd      vcal[i].rgbBlue += 0x5555;nd    }nd    ify(i == 6) { /* Smageal excep.org eelybrown D/ a      vcal[i].rgbG (Gn >>= 1;nd    }nd  }nd  
  eely(i=0; i< 256; i++) {nd    color [i].rgbRfi =y(vcal[i & 0xf].rgbRfi / 5)*3 a      +y(vcal[i >> 4].rgbRfi / 5)*2;nd    color [i].rgbG (Gn =y(vcal[i & 0xf].rgbG (Gn / 5)*3 a      +y(vcal[i >> 4].rgbG (Gn / 5)*2;nd    color [i].rgbBlue =y(vcal[i & 0xf].rgbBlue / 5)*3 a      +y(vcal[i >> 4].rgbBlue / 5)*2;nd    color [i].rgbR

 Thi =y0;nd  }

  memcpy (pPal, color, 4*256);

  Hbovif_PaletteInvalidn fCfree mhhPal);
}

on coFAR PASCAL WndProc (HWND hWnd, UINT messing, WPARAM wParN , LPARAM lParN )
{

  return DefWrraowProc (hWnd, messing, wParN , lParN );
}

#defice SPRINTF_DDERR(x,y) toprox : spot tfy(bufe d);ybreak
#defice TRACE_DDERR(x) SPRINTF_DDERR(x,#x)ndnddin cauvoiimTrapoLastDDrawErrely(HRESULT hR

ulheloserr*buf)
{
  sbutre mhR

ulh)
  {nd    TRACE_DDERR (DDERR_ALREADYINITIALIZED);nd    TRACE_DDERR (DDERR_CANNOTATTACHSURFACE);nd    TRACE_DDERR (DDERR_CANNOTDETACHSURFACE);nd    TRACE_DDERR (DDERR_CURRENTLYNOTAVAIL);nd    TRACE_DDERR (DDERR_EXCEPTION);nd    TRACE_DDERR (DDERR_GENERIC);nd    TRACE_DDERR (DDERR_HEIGHTALIGN);nd    TRACE_DDERR (DDERR_INCOMPATIBLEPRIMARY);nd    TRACE_DDERR (DDERR_INVALIDCAPS);nd    TRACE_DDERR (DDERR_INVALIDCLIPLIST);nd    TRACE_DDERR (DDERR_INVALIDMODE);nd    TRACE_DDERR (DDERR_INVALIDOBJECT);nd    TRACE_DDERR (DDERR_INVALIDPARAMS);nd    TRACE_DDERR (DDERR_INVALIDPIXELFORMAT);nd    TRACE_DDERR (DDERR_INVALIDRECT);nd    TRACE_DDERR (DDERR_LOCKEDSURFACES);nd    TRACE_DDERR (DDERR_NO3D);nd    TRACE_DDERR (DDERR_NOALPHAHW);nd    TRACE_DDERR (DDERR_NOCLIPLIST);nd    TRACE_DDERR (DDERR_NOCOLORCONVHW);nd    TRACE_DDERR (DDERR_NOCOOPERATIVELEVELSET);nd    TRACE_DDERR (DDERR_NOCOLORKEY);nd    TRACE_DDERR (DDERR_NOCOLORKEYHW);nd    TRACE_DDERR (DDERR_NODIRECTDRAWSUPPORT);nd    TRACE_DDERR (DDERR_NOEXCLUSIVEMODE);nd    TRACE_DDERR (DDERR_NOFLIPHW);nd    TRACE_DDERR (DDERR_NOGDI);nd    TRACE_DDERR (DDERR_NOMIRRORHW);nd    TRACE_DDERR (DDERR_NOTFOUND);nd    TRACE_DDERR (DDERR_NOOVERLAYHW);nd    TRACE_DDERR (DDERR_NORASTEROPHW);nd    TRACE_DDERR (DDERR_NOROTATIONHW);nd    TRACE_DDERR (DDERR_NOSTRETCHHW);nd    TRACE_DDERR (DDERR_NOT4BITCOLOR);nd    TRACE_DDERR (DDERR_NOT4BITCOLORINDEX);nd    TRACE_DDERR (DDERR_NOT8BITCOLOR);nd    TRACE_DDERR (DDERR_NOTEXTUREHW);nd    TRACE_DDERR (DDERR_NOVSYNCHW);nd    TRACE_DDERR (DDERR_NOZBUFFERHW);nd    TRACE_DDERR (DDERR_NOZOVERLAYHW);nd    TRACE_DDERR (DDERR_OUTOFCAPS);nd    TRACE_DDERR (DDERR_OUTOFMEMORY);nd    TRACE_DDERR (DDERR_OUTOFVIDEOMEMORY);nd    TRACE_DDERR (DDERR_OVERLAYCANTCLIP);nd    TRACE_DDERR (DDERR_OVERLAYCOLORKEYONLYONEACTIVE);nd    TRACE_DDERR (DDERR_PALETTEBUSY);nd    TRACE_DDERR (DDERR_COLORKEYNOTSET);nd    TRACE_DDERR (DDERR_SURFACEALREADYATTACHED);nd    TRACE_DDERR (DDERR_SURFACEALREADYDEPENDENT);nd    TRACE_DDERR (DDERR_SURFACEBUSY);nd    TRACE_DDERR (DDERR_CANTLOCKSURFACE);nd    TRACE_DDERR (DDERR_SURFACEISOBSCURED);nd    TRACE_DDERR (DDERR_SURFACELOST);nd    TRACE_DDERR (DDERR_SURFACENOTATTACHED);nd    TRACE_DDERR (DDERR_TOOBIGHEIGHT);nd    TRACE_DDERR (DDERR_TOOBIGSIZE);nd    TRACE_DDERR (DDERR_TOOBIGWIDTH);nd    TRACE_DDERR (DDERR_UNSUPPORTED);nd    TRACE_DDERR (DDERR_UNSUPPORTEDFORMAT);nd    TRACE_DDERR (DDERR_UNSUPPORTEDMASK);nd    TRACE_DDERR (DDERR_VERTICALBLANKINPROGRESS);nd    TRACE_DDERR (DDERR_WASSTILLDRAWING);nd    TRACE_DDERR (DDERR_XALIGN);nd    TRACE_DDERR (DDERR_INVALIDDIRECTDRAWGUID);nd    TRACE_DDERR (DDERR_DIRECTDRAWALREADYCREATED);nd    TRACE_DDERR (DDERR_NODIRECTDRAWHW);nd    TRACE_DDERR (DDERR_PRIMARYSURFACEALREADYEXISTS);nd    TRACE_DDERR (DDERR_NOEMULATION);nd    TRACE_DDERR (DDERR_REGIONTOOSMALL);nd    TRACE_DDERR (DDERR_CLIPPERISUSINGHWND);nd    TRACE_DDERR (DDERR_NOCLIPPERATTACHED);nd    TRACE_DDERR (DDERR_NOHWND);nd    TRACE_DDERR (DDERR_HWNDSUBCLASSED);nd    TRACE_DDERR (DDERR_HWNDALREADYSET);nd    TRACE_DDERR (DDERR_NOPALETTEATTACHED);nd    TRACE_DDERR (DDERR_NOPALETTEHW);nd    TRACE_DDERR (DDERR_BLTFASTCANTCLIP);nd    TRACE_DDERR (DDERR_NOBLTHW);nd    TRACE_DDERR (DDERR_NODDROPSHW);nd    TRACE_DDERR (DDERR_OVERLAYNOTVISIBLE);nd    TRACE_DDERR (DDERR_NOOVERLAYDEST);nd    TRACE_DDERR (DDERR_INVALIDPOSITION);nd    TRACE_DDERR (DDERR_NOTAOVERLAYSURFACE);nd    TRACE_DDERR (DDERR_EXCLUSIVEMODEALREADYSET);nd    TRACE_DDERR (DDERR_NOTFLIPPABLE);nd    TRACE_DDERR (DDERR_CANTDUPLICATE);nd    TRACE_DDERR (DDERR_NOTLOCKED);nd    TRACE_DDERR (DDERR_CANTCREATEDC);nd    TRACE_DDERR (DDERR_NODC);nd    TRACE_DDERR (DDERR_WRONGMODE);nd    TRACE_DDERR (DDERR_IMPLICITLYCREATED);nd    TRACE_DDERR (DDERR_NOTPALETTIZED);nd    TRACE_DDERR (DDERR_UNSUPPORTEDMODE);nd    TRACE_DDERR (DDERR_NOMIPMAPHW);nd    TRACE_DDERR (DDERR_INVALIDSURFACETYPE);nd    TRACE_DDERR (DDERR_DCALREADYCREATED);nd    TRACE_DDERR (DDERR_CANTPAGELOCK);nd    TRACE_DDERR (DDERR_CANTPAGEUNLOCK);nd    TRACE_DDERR (DDERR_NOTPAGELOCKED);nd    TRACE_DDERR (DDERR_NOTINITIALIZED);nd    defaulh : spot tfy(bufe "Unknown DirectDraw errely%i\n", hR

ulh);nd  } 
}

t t
DDrawFailurey(HRESULT hr)
{
  oserrbuf [128];
  TrapoLastDDrawErrely(hr,rbuf);nd  pot tfy("DirectDraw initializu.org failed: %s\n", buf);nd  return 1;
}

t t
stadtOpen(din f_t *s)
{
  WNDCLASS wc;nd  rNtohr;nd  DDSURFACEDESC ddsd;nd  DDPIXELFORMAT ddpf;

  /* Registeriwrraowaclass D/ a  wc.style         = CS_HREDRAW | CS_VREDRAW; a  wc.lpfnWndProc   = WndProc; a  wc.cbClsExtra    = 0; a  wc.cbWndExtra    = 0; a  wc.hIndinnce     = NULL;nd  wc.hIcrg         = LoadIcrg (NULL, IDI_APPLICATION);nd  wc.hCursely      = LoadCursely(NULL, IDC_ARROW);nd  wc.hbrBackground = NULL;nd  wc.lpszMenuName  = NULL;nd  wc.lpszClassName = "e (Gsci.WndClass";nd  RegisterClass (&wc);

  /* Crcn f d1lishowawrraowaD/ a  hMlicWnd = Crcn fWrraowEx (nd    0,nd    "e (Gsci.WndClass",nd    "e (Gsci",nd    WS_POPUP,nd    0,nd    0,nd    640,nd    400,nd    NULL,nd    NULL,nd    NULL,nd    NULLnd  );

  ify(!hMlicWnd) return 1;

  ShowWrraowy(hMlicWnd, SW_SHOW);nd  Updn fWrraowy(hMlicWnd);nd  SetFocusy(hMlicWnd);nd
  /* Initialize DirectDraw foriwrraowedgenee, crcn fss fosurfapoud1lind     attfre clipperr*/
  ify(!pDD)
  {nd    hr=DirectDrawCrcn fs(NULL, &pDD, NULL);nd    ify(!pDD) return DDrawFailurey(hr);nd  }nd
  hr=IDirectDraw_SetCooper1deroLevel (pDD, hMlicWnd, DDSCL_NORMAL);nd  ify(hr != DD_OK) return DDrawFailurey(hr);ndnd  ddsd.dwSize = sizeof (DDSURFACEDESC);nd  ddsd.dwFlags = DDSD_CAPS;nd  ddsd.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE;nd
  hr=IDirectDraw_Crcn fSurfapou(pDD, &ddsd, &pPrimary, NULL);nd  ify(hr != DD_OK) return DDrawFailurey(hr);ndnd  hr=IDirectDraw_Crcn fClipperr(pDD, 0, &pClipper, NULL);nd  ify(hr != DD_OK) return DDrawFailurey(hr);ndnd  hr=IDirectDrawClipper_SetHWnd (pClipper, 0, hMlicWnd);nd  ify(hr != DD_OK) return DDrawFailurey(hr);ndnd  hr=IDirectDrawSurfapo_SetClipperr(pPrimary, pClipper);nd  ify(hr != DD_OK) return DDrawFailurey(hr);ndnd  /* Initialize Hbovifr*/
  Hbovif_Init();nd  hhPalette = Hbovif_PaletteIndinnce();nd  ify(!hhPalette) return 1;
 minitColors (hhPalette);nd  hhConverabo = Hbovif_ConveraboIndinnce (0);nd  ify(!hhConverabo) return 1;
 mhfSrc = Hbovif_F.gnu.Newy(8, 0, 0, 0, 0, 1);nd  nd  /* Frra not1s fopixel w.gnu.odifs foprimaryosurfapoud1liconvera ittnesHbovifnd     w.gnu.o*/
  ddpf.dwSize = sizeof (DDPIXELFORMAT);nd  IDirectDrawSurfapo_GetPixelF.gnu. (pPrimary, &ddpf);nd  ify(ddpf.dwFlags & DDPF_PALETTEINDEXED8)
  {nd    pot tfy("Desktop impin 8 bppgenee - curre hlyment,supported\n");nd    return 1;
 m}
 mhfDest = Hbovif_F.gnu.Newy(ddpf.dwRGBBitCount,nd    ddpf.dwRBitMask, ddpf.dwGBitMask, ddpf.dwBBitMask, 0, 0);nd  ndnd  /* Setmstadt

 call frko*/
  s->stadt

call frk = stadt

call frk
ddraw;

  return 0;
} andvoii
stadtClose(din f_t *s)
{
  /* Deinitialize Hbovifr*/
  Hbovif_ConveraboReturn (hhConverabo);
  Hbovif_PaletteReturn (hhPalette);nd  Hbovif_Done();nd
  /* Deinitialize DirectDraw */
  ify(pDD)
  {nd    ify(pClipper)nd    {nd     uIDirectDrawClipper_Reis pro(pClipper);nd      pClipperr= NULL;nd    }nd
    ify(pPrimary)nd    {nd     uIDirectDrawSurfapo_Reis pro(pPrimary);nd      pPrimaryr= NULL;nd    }nd   uIDirectDraw_Reis pro(pDD);nd    pDDr= NULL;nd  }
} and/U D Gtadt

 call frko* D/ andvoiindstadt

draw_region
ddraw(by f *dn a,nd                           rNtosx, rNtosthor			   rNtox, rNtoy, rNtoxl, rNtoyl)
{
  rNtoxend, yend;nd  DDSURFACEDESC ddsd;nd
  /* adjustofos lonn fs */
  ify(x < 0) {nd    xl += x;nd    x =y0;nd  }

  ify(y < 0) {nd    yl += y;nd    y =y0;nd  }

  xend =yx + xl + 1;
 myend =yy + yl + 1;

  ify(xend > SCI_SCREEN_WIDTH)nd    xend =ySCI_SCREEN_WIDTH;

  ify(yend > SCI_SCREEN_HEIGHT)nd    yend =ySCI_SCREEN_HEIGHT;

  ify(IDirectDrawSurfapo_IsLos. (pPrimary) == DDERR_SURFACELOST)nd   uIDirectDrawSurfapo_Reed rro(pPrimary);ndnd  ddsd.dwSize = sizeof (DDSURFACEDESC);nd  IDirectDrawSurfapo_Lorko(pPrimary, NULL, &ddsd, DDLOCK_WAIT, NULL);nd
  Hbovif_ConveraboRequest (hhConverabo,mhfSrc,mhfDest);nd  Hbovif_ConveraboPalette (hhConverabo,mhhPalette,mhhPalette);nd  Hbovif_Converabos pr (hhConverabo,nd    dn a,nd    sx, sthoxl, yl,nd    xl,nd    ddsd.lpSurfapo,nd    x*2, y*2, xl*2, yl*2,nd    ddsd.lPutre);nd  nd  IDirectDrawSurfapo_Unlorko(pPrimary, NULL);
} andvoii
stadt

call frk
ddraw (structs_din f *s, rNtofommand, rNtox, rNtoy, rNtoxl, rNtoyl)
{
  rNtomp_x, mp_y, mp_size_x, mp_size_y;

  ify(s->mouse_porNtbo) {nd    mp_x = s->porNtbo_x - s->mouse_porNtbo->hot_x;nd    mp_y = s->porNtbo_y - s->mouse_porNtbo->hot_y;nd    mp_size_x = s->mouse_porNtbo->size_x;nd    mp_size_y = s->mouse_porNtbo->size_y;nd  } elpro{ /* No mouse porNtbo D/ a    mp_x = s->porNtbo_x;nd    mp_y = s->porNtbo_y;nd    mp_size_x = mp_size_y = 0;nd  }


  sbutre mfommand) {nd  toproGRAPHICS_CALLBACK_REDRAW_ALL:nd    stadt

draw_region
ddraw(s->pic->viewhor			       0, 0, 0, 0, 320, 200);nd    ify(s->mouse_porNtbo)nd      stadt

draw_region
ddraw(s->mouse_porNtbo->bitmap,nd                                 0, 0, s->porNtbo_x, s->porNtbo_y,nd                                 s->mouse_porNtbo->size_x, s->mouse_porNtbo->size_y);nd    break;

  toproGRAPHICS_CALLBACK_REDRAW_BOX:nd    stadt

draw_region
ddraw(s->pic->viewh /* Draw box D/ a			       x, thox, thoxl, yl);nd    ify(s->mouse_porNtbo)nd      stadt

draw_region
ddraw(s->mouse_porNtbo->bitmap,nd                                 0, 0, s->porNtbo_x, s->porNtbo_y,nd                                 s->mouse_porNtbo->size_x, s->mouse_porNtbo->size_y);nd    break;

  toproGRAPHICS_CALLBACK_REDRAW_POINTER:nd    stadt

draw_region
ddraw(s->pic->viewh /* Remereiold porNtbo D/ a                               s->last_porNtbo_x,s->last_porNtbo_yhor			       s->last_porNtbo_x,s->last_porNtbo_yhor			       s->last_porNtbo_size_x, s->last_porNtbo_size_y);nd    ify(s->mouse_porNtbo)nd      stadt

draw_region
ddraw(s->mouse_porNtbo->bitmap, /* Draw newyporNtbo D/ a                                 0, 0, s->porNtbo_x, s->porNtbo_y,nd                                 s->mouse_porNtbo->size_x, s->mouse_porNtbo->size_y);nd    break;
defaulh:nd    fpot tf(stderr,"stadt

call frk
ddraw: Invalidofommand %d\n", fommand);nd  }

  s->last_porNtbo_size_x = mp_size_x;nd  s->last_porNtbo_size_y = mp_size_y;nd  s->last_porNtbo_x = mp_x;nd  s->last_porNtbo_y = mp_y; /* Updn f mouse porNtbo din us */
}

#endify/* HAVE_DDRAW */
