# Microsoft Developer Studio Project File - Name="fsci_dll" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=fsci_dll - Win32 Purify
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "fsci_dll.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "fsci_dll.mak" CFG="fsci_dll - Win32 Purify"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "fsci_dll - Win32 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "fsci_dll - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "fsci_dll - Win32 Purify" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "fsci_dll - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "fsci_dll_Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "FSCI_DLL_EXPORTS" /YX /FD /c
# ADD CPP /nologo /MD /W3 /GX /Zi /O1 /Ob2 /I "..\..\..\SDL\include" /I "..\include" /I "..\include\win32" /D PACKAGE=\"FreeSCI\" /D VERSION=__TIMESTAMP__ /D "X_DISPLAY_MISSING" /D "HAVE_SDL" /D "HAVE_STRING_H" /D "_CONSOLE" /D "WIN32" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "FREESCI_EXPORTS" /D "NDEBUG" /FR /YX /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0xc09 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /machine:I386
# ADD LINK32 ..\..\..\SDL\lib\SDL.lib winmm.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib /nologo /dll /debug /machine:I386 /out:"..\..\bin\fsci.dll" /fixed:no
# SUBTRACT LINK32 /pdb:none

!ELSEIF  "$(CFG)" == "fsci_dll - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "fsci_dll_Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "FSCI_DLL_EXPORTS" /YX /FD /GZ /c
# ADD CPP /nologo /MDd /W3 /Gm /Gi /GR /GX /ZI /Od /I "..\..\..\SDL\include" /I "..\include" /I "..\include\win32" /D PACKAGE=\"FreeSCI\" /D "X_DISPLAY_MISSING" /D "HAVE_SDL" /D "_WINDOWS" /D "_USRDLL" /D "FREESCI_EXPORTS" /D VERSION=__TIMESTAMP__ /D "_DEBUG" /D "WIN32" /D "_CONSOLE" /D "_MBCS" /D "HAVE_STRING_H" /D "DEBUG_SOUND_SERVER" /FR /YX /FD /GZ /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0xc09 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /debug /machine:I386 /pdbtype:sept
# ADD LINK32 ..\..\..\SDL\lib\SDL.lib winmm.lib kernel32.lib user32.lib gdi32.lib advapi32.lib uuid.lib ole32.lib /nologo /dll /debug /machine:I386 /out:"Debug\fsci.dll" /pdbtype:sept
# SUBTRACT LINK32 /pdb:none /map

!ELSEIF  "$(CFG)" == "fsci_dll - Win32 Purify"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "fsci_dll___Win32_Purify"
# PROP BASE Intermediate_Dir "fsci_dll___Win32_Purify"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Purify"
# PROP Intermediate_Dir "fsci_dll_Purify"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MDd /W3 /Gm /Gi /GR /GX /Zi /Od /I "..\..\..\SDL\include" /I "..\include" /I "..\include\win32" /D PACKAGE=\"FreeSCI\" /D "X_DISPLAY_MISSING" /D "HAVE_SDL" /D "_WINDOWS" /D "_USRDLL" /D "FREESCI_EXPORTS" /D VERSION=__TIMESTAMP__ /D "_DEBUG" /D "WIN32" /D "_CONSOLE" /D "_MBCS" /D "HAVE_STRING_H" /D "DEBUG_SOUND_SERVER" /D "SATISFY_PURIFY" /FR /YX /FD /GZ /c
# ADD CPP /nologo /MT /W3 /Gm /Gi /GR /GX /Zi /Od /I "..\..\..\SDL\include" /I "..\include" /I "..\include\win32" /D PACKAGE=\"FreeSCI\" /D "X_DISPLAY_MISSING" /D "HAVE_SDL" /D "_WINDOWS" /D "_USRDLL" /D "FREESCI_EXPORTS" /D VERSION=__TIMESTAMP__ /D "_DEBUG" /D "WIN32" /D "_CONSOLE" /D "_MBCS" /D "HAVE_STRING_H" /D "SATISFY_PURIFY" /FR /YX /FD /GZ /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 dxguid.lib dxerr8.lib msvcrtd.lib ..\..\..\SDL\lib\SDL.lib winmm.lib kernel32.lib user32.lib gdi32.lib advapi32.lib uuid.lib ole32.lib /nologo /dll /pdb:none /map /debug /machine:I386 /nodefaultlib:"libc.lib" /nodefaultlib:"libcmt.lib" /nodefaultlib:"msvcrt.lib" /nodefaultlib:"libcd.lib" /nodefaultlib:"libcmtd.lib" /out:"Purify/fsci.dll"
# ADD LINK32 ..\..\..\SDL\lib\SDL.lib winmm.lib kernel32.lib user32.lib gdi32.lib advapi32.lib uuid.lib ole32.lib /nologo /dll /incremental:no /map:"Purify/fsci.map" /debug /machine:I386 /out:"Purify\fsci.dll" /fixed:no
# SUBTRACT LINK32 /pdb:none

!ENDIF 

# Begin Target

# Name "fsci_dll - Win32 Release"
# Name "fsci_dll - Win32 Debug"
# Name "fsci_dll - Win32 Purify"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Group "engine"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\engine\game.c
# End Source File
# Begin Source File

SOURCE=..\engine\grammar.c
# End Source File
# Begin Source File

SOURCE=..\engine\heap.c
# End Source File
# Begin Source File

SOURCE=..\engine\kernel.c
# End Source File
# Begin Source File

SOURCE=..\engine\kevent.c
# End Source File
# Begin Source File

SOURCE=..\engine\kfile.c
# End Source File
# Begin Source File

SOURCE=..\engine\kgraphics.c
# End Source File
# Begin Source File

SOURCE=..\engine\klists.c
# End Source File
# Begin Source File

SOURCE=..\engine\kmath.c
# End Source File
# Begin Source File

SOURCE=..\engine\kmenu.c
# End Source File
# Begin Source File

SOURCE=..\engine\kmovement.c
# End Source File
# Begin Source File

SOURCE=..\engine\kscripts.c
# End Source File
# Begin Source File

SOURCE=..\engine\ksound.c
# End Source File
# Begin Source File

SOURCE=..\engine\kstring.c
# End Source File
# Begin Source File

SOURCE=..\engine\said.c
# End Source File
# Begin Source File

SOURCE=..\engine\savegame.c
# End Source File
# Begin Source File

SOURCE=..\engine\savegame.cfsml
# End Source File
# Begin Source File

SOURCE=..\engine\scriptdebug.c
# End Source File
# Begin Source File

SOURCE=..\engine\simplesaid.c
# End Source File
# Begin Source File

SOURCE=..\engine\vm.c
# End Source File
# End Group
# Begin Group "gfx"

# PROP Default_Filter ""
# Begin Group "resource"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\gfx\resource\sci_cursor_0.c
# End Source File
# Begin Source File

SOURCE=..\gfx\resource\sci_font.c
# End Source File
# Begin Source File

SOURCE=..\gfx\resource\sci_pal_1.c
# End Source File
# Begin Source File

SOURCE=..\gfx\resource\sci_pic_0.c
# End Source File
# Begin Source File

SOURCE=..\gfx\resource\sci_resmgr.c
# End Source File
# Begin Source File

SOURCE=..\gfx\resource\sci_view_0.c
# End Source File
# Begin Source File

SOURCE=..\gfx\resource\sci_view_1.c
# End Source File
# End Group
# Begin Group "drivers"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\gfx\drivers\dd_driver.cpp
# End Source File
# Begin Source File

SOURCE=..\gfx\drivers\dd_driver_line.cpp
# End Source File
# Begin Source File

SOURCE=..\gfx\drivers\gfx_drivers.c
# End Source File
# Begin Source File

SOURCE=..\gfx\drivers\sdl_driver.c
# End Source File
# End Group
# Begin Source File

SOURCE=..\gfx\alpha_mvi_crossblit.c
# End Source File
# Begin Source File

SOURCE=..\gfx\antialias.c
# End Source File
# Begin Source File

SOURCE=..\gfx\font.c
# End Source File
# Begin Source File

SOURCE=..\gfx\gfx_crossblit.c

!IF  "$(CFG)" == "fsci_dll - Win32 Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "fsci_dll - Win32 Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "fsci_dll - Win32 Purify"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\gfx\gfx_line.c

!IF  "$(CFG)" == "fsci_dll - Win32 Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "fsci_dll - Win32 Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "fsci_dll - Win32 Purify"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\gfx\gfx_pixmap_scale.c

!IF  "$(CFG)" == "fsci_dll - Win32 Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "fsci_dll - Win32 Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "fsci_dll - Win32 Purify"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\gfx\gfx_resource.c
# End Source File
# Begin Source File

SOURCE=..\gfx\gfx_support.c
# End Source File
# Begin Source File

SOURCE=..\gfx\gfx_tools.c
# End Source File
# Begin Source File

SOURCE=..\gfx\menubar.c
# End Source File
# Begin Source File

SOURCE=..\gfx\operations.c
# End Source File
# Begin Source File

SOURCE=..\gfx\resmgr.c
# End Source File
# Begin Source File

SOURCE=..\gfx\sbtree.c
# End Source File
# Begin Source File

SOURCE=..\gfx\sci_widgets.c
# End Source File
# Begin Source File

SOURCE=..\gfx\widgets.c
# End Source File
# Begin Source File

SOURCE=..\gfx\wrapper.c
# End Source File
# End Group
# Begin Group "scicore"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\scicore\console.c
# End Source File
# Begin Source File

SOURCE=..\scicore\decompress0.c
# End Source File
# Begin Source File

SOURCE=..\scicore\decompress01.c
# End Source File
# Begin Source File

SOURCE=..\scicore\decompress1.c
# End Source File
# Begin Source File

SOURCE=..\scicore\decompress11.c
# End Source File
# Begin Source File

SOURCE=..\scicore\old_objects.c
# End Source File
# Begin Source File

SOURCE=..\scicore\resource.c
# End Source File
# Begin Source File

SOURCE=..\scicore\resource_map.c
# End Source File
# Begin Source File

SOURCE=..\scicore\resource_patch.c
# End Source File
# Begin Source File

SOURCE=..\scicore\resourcecheck.c
# End Source File
# Begin Source File

SOURCE=..\scicore\sci_memory.c
# End Source File
# Begin Source File

SOURCE=..\scicore\script.c
# End Source File
# Begin Source File

SOURCE=..\scicore\tools.c
# End Source File
# Begin Source File

SOURCE=..\scicore\vocab.c
# End Source File
# Begin Source File

SOURCE=..\scicore\vocab_debug.c
# End Source File
# End Group
# Begin Group "sound"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\sound\event_ss.c

!IF  "$(CFG)" == "fsci_dll - Win32 Release"

!ELSEIF  "$(CFG)" == "fsci_dll - Win32 Debug"

# ADD CPP /W4

!ELSEIF  "$(CFG)" == "fsci_dll - Win32 Purify"

# ADD BASE CPP /W4
# ADD CPP /W3

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\sound\event_ss_win32.c

!IF  "$(CFG)" == "fsci_dll - Win32 Release"

!ELSEIF  "$(CFG)" == "fsci_dll - Win32 Debug"

# ADD CPP /W4

!ELSEIF  "$(CFG)" == "fsci_dll - Win32 Purify"

# ADD BASE CPP /W4
# ADD CPP /W3

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\sound\midi_adlib.c

!IF  "$(CFG)" == "fsci_dll - Win32 Release"

!ELSEIF  "$(CFG)" == "fsci_dll - Win32 Debug"

!ELSEIF  "$(CFG)" == "fsci_dll - Win32 Purify"

# ADD CPP /W3

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\sound\midi_device.c

!IF  "$(CFG)" == "fsci_dll - Win32 Release"

!ELSEIF  "$(CFG)" == "fsci_dll - Win32 Debug"

!ELSEIF  "$(CFG)" == "fsci_dll - Win32 Purify"

# ADD CPP /W3

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\sound\midi_mt32.c

!IF  "$(CFG)" == "fsci_dll - Win32 Release"

!ELSEIF  "$(CFG)" == "fsci_dll - Win32 Debug"

!ELSEIF  "$(CFG)" == "fsci_dll - Win32 Purify"

# ADD CPP /W3

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\sound\midi_mt32gm.c

!IF  "$(CFG)" == "fsci_dll - Win32 Release"

!ELSEIF  "$(CFG)" == "fsci_dll - Win32 Debug"

!ELSEIF  "$(CFG)" == "fsci_dll - Win32 Purify"

# ADD CPP /W3

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\sound\midiout.c

!IF  "$(CFG)" == "fsci_dll - Win32 Release"

!ELSEIF  "$(CFG)" == "fsci_dll - Win32 Debug"

!ELSEIF  "$(CFG)" == "fsci_dll - Win32 Purify"

# ADD CPP /W3

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\sound\midiout_win32mci.c

!IF  "$(CFG)" == "fsci_dll - Win32 Release"

!ELSEIF  "$(CFG)" == "fsci_dll - Win32 Debug"

!ELSEIF  "$(CFG)" == "fsci_dll - Win32 Purify"

# ADD CPP /W3

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\sound\oldmidi.c

!IF  "$(CFG)" == "fsci_dll - Win32 Release"

!ELSEIF  "$(CFG)" == "fsci_dll - Win32 Debug"

!ELSEIF  "$(CFG)" == "fsci_dll - Win32 Purify"

# ADD CPP /W3

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\sound\polled_ss.c

!IF  "$(CFG)" == "fsci_dll - Win32 Release"

!ELSEIF  "$(CFG)" == "fsci_dll - Win32 Debug"

# ADD CPP /W4

!ELSEIF  "$(CFG)" == "fsci_dll - Win32 Purify"

# ADD BASE CPP /W4
# ADD CPP /W3

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\sound\polled_ss_sdl.c

!IF  "$(CFG)" == "fsci_dll - Win32 Release"

!ELSEIF  "$(CFG)" == "fsci_dll - Win32 Debug"

# ADD CPP /W4

!ELSEIF  "$(CFG)" == "fsci_dll - Win32 Purify"

# ADD BASE CPP /W4
# ADD CPP /W3

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\sound\polled_ss_unix.c

!IF  "$(CFG)" == "fsci_dll - Win32 Release"

!ELSEIF  "$(CFG)" == "fsci_dll - Win32 Debug"

# ADD CPP /W4

!ELSEIF  "$(CFG)" == "fsci_dll - Win32 Purify"

# ADD BASE CPP /W4
# ADD CPP /W3

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\sound\polled_ss_win32.c

!IF  "$(CFG)" == "fsci_dll - Win32 Release"

!ELSEIF  "$(CFG)" == "fsci_dll - Win32 Debug"

# ADD CPP /W4

!ELSEIF  "$(CFG)" == "fsci_dll - Win32 Purify"

# ADD BASE CPP /W4
# ADD CPP /W3

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\sound\sfx_save.c

!IF  "$(CFG)" == "fsci_dll - Win32 Release"

!ELSEIF  "$(CFG)" == "fsci_dll - Win32 Debug"

# ADD CPP /W4

!ELSEIF  "$(CFG)" == "fsci_dll - Win32 Purify"

# ADD CPP /W3

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\sound\sfx_save.cfsml
# End Source File
# Begin Source File

SOURCE=..\sound\sound.c

!IF  "$(CFG)" == "fsci_dll - Win32 Release"

!ELSEIF  "$(CFG)" == "fsci_dll - Win32 Debug"

# ADD CPP /W4

!ELSEIF  "$(CFG)" == "fsci_dll - Win32 Purify"

# ADD BASE CPP /W4
# ADD CPP /W3

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\sound\soundserver.c

!IF  "$(CFG)" == "fsci_dll - Win32 Release"

!ELSEIF  "$(CFG)" == "fsci_dll - Win32 Debug"

# ADD CPP /W4

!ELSEIF  "$(CFG)" == "fsci_dll - Win32 Purify"

# ADD BASE CPP /W4
# ADD CPP /W3

!ENDIF 

# End Source File
# End Group
# Begin Source File

SOURCE=.\usleep.c
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Group "win32"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\gfx\drivers\dd_driver.h
# End Source File
# Begin Source File

SOURCE=..\include\graphics_ddraw.h
# End Source File
# Begin Source File

SOURCE=..\include\win32\messages.h
# End Source File
# End Group
# Begin Group "SDL"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\..\SDL\include\begin_code.h
# End Source File
# Begin Source File

SOURCE=..\..\..\SDL\include\close_code.h
# End Source File
# Begin Source File

SOURCE=..\..\..\SDL\include\SDL.h
# End Source File
# Begin Source File

SOURCE=..\..\..\SDL\include\SDL_active.h
# End Source File
# Begin Source File

SOURCE=..\..\..\SDL\include\SDL_audio.h
# End Source File
# Begin Source File

SOURCE=..\..\..\SDL\include\SDL_byteorder.h
# End Source File
# Begin Source File

SOURCE=..\..\..\SDL\include\SDL_cdrom.h
# End Source File
# Begin Source File

SOURCE=..\..\..\SDL\include\SDL_copying.h
# End Source File
# Begin Source File

SOURCE=..\..\..\SDL\include\SDL_endian.h
# End Source File
# Begin Source File

SOURCE=..\..\..\SDL\include\SDL_error.h
# End Source File
# Begin Source File

SOURCE=..\..\..\SDL\include\SDL_events.h
# End Source File
# Begin Source File

SOURCE=..\..\..\SDL\include\SDL_getenv.h
# End Source File
# Begin Source File

SOURCE=..\..\..\SDL\include\SDL_joystick.h
# End Source File
# Begin Source File

SOURCE=..\..\..\SDL\include\SDL_keyboard.h
# End Source File
# Begin Source File

SOURCE=..\..\..\SDL\include\SDL_keysym.h
# End Source File
# Begin Source File

SOURCE=..\..\..\SDL\include\SDL_main.h
# End Source File
# Begin Source File

SOURCE=..\..\..\SDL\include\SDL_mouse.h
# End Source File
# Begin Source File

SOURCE=..\..\..\SDL\include\SDL_mutex.h
# End Source File
# Begin Source File

SOURCE=..\..\..\SDL\include\SDL_opengl.h
# End Source File
# Begin Source File

SOURCE=..\..\..\SDL\include\SDL_quit.h
# End Source File
# Begin Source File

SOURCE=..\..\..\SDL\include\SDL_rwops.h
# End Source File
# Begin Source File

SOURCE=..\..\..\SDL\include\SDL_syswm.h
# End Source File
# Begin Source File

SOURCE=..\..\..\SDL\include\SDL_thread.h
# End Source File
# Begin Source File

SOURCE=..\..\..\SDL\include\SDL_timer.h
# End Source File
# Begin Source File

SOURCE=..\..\..\SDL\include\SDL_types.h
# End Source File
# Begin Source File

SOURCE=..\..\..\SDL\include\SDL_version.h
# End Source File
# Begin Source File

SOURCE=..\..\..\SDL\include\SDL_video.h
# End Source File
# End Group
# Begin Source File

SOURCE=..\include\console.h
# End Source File
# Begin Source File

SOURCE=..\include\engine.h
# End Source File
# Begin Source File

SOURCE=..\include\event.h
# End Source File
# Begin Source File

SOURCE=..\include\gfx_driver.h
# End Source File
# Begin Source File

SOURCE=..\include\gfx_drivers_list.h
# End Source File
# Begin Source File

SOURCE=..\include\gfx_operations.h
# End Source File
# Begin Source File

SOURCE=..\include\gfx_options.h
# End Source File
# Begin Source File

SOURCE=..\include\gfx_resmgr.h
# End Source File
# Begin Source File

SOURCE=..\include\gfx_resource.h
# End Source File
# Begin Source File

SOURCE=..\include\gfx_sci.h
# End Source File
# Begin Source File

SOURCE=..\include\gfx_state.h
# End Source File
# Begin Source File

SOURCE=..\include\gfx_state_internal.h
# End Source File
# Begin Source File

SOURCE=..\include\gfx_system.h
# End Source File
# Begin Source File

SOURCE=..\include\gfx_tools.h
# End Source File
# Begin Source File

SOURCE=..\include\gfx_widgets.h
# End Source File
# Begin Source File

SOURCE=..\include\graphics.h
# End Source File
# Begin Source File

SOURCE=..\include\graphics_png.h
# End Source File
# Begin Source File

SOURCE=..\include\heap.h
# End Source File
# Begin Source File

SOURCE=..\include\kdebug.h
# End Source File
# Begin Source File

SOURCE=..\include\kernel.h
# End Source File
# Begin Source File

SOURCE=..\include\menubar.h
# End Source File
# Begin Source File

SOURCE=..\include\midi_device.h
# End Source File
# Begin Source File

SOURCE=..\include\midiout.h
# End Source File
# Begin Source File

SOURCE=..\include\modules.h
# End Source File
# Begin Source File

SOURCE=..\include\old_objects.h
# End Source File
# Begin Source File

SOURCE=..\include\resource.h
# End Source File
# Begin Source File

SOURCE=..\include\sbtree.h
# End Source File
# Begin Source File

SOURCE=..\include\sci_conf.h
# End Source File
# Begin Source File

SOURCE=..\include\sci_dos.h
# End Source File
# Begin Source File

SOURCE=..\include\sci_graphics.h
# End Source File
# Begin Source File

SOURCE=..\include\sci_memory.h
# End Source File
# Begin Source File

SOURCE=..\include\sci_widgets.h
# End Source File
# Begin Source File

SOURCE=..\include\sciresource.h
# End Source File
# Begin Source File

SOURCE=..\include\scitypes.h
# End Source File
# Begin Source File

SOURCE=..\include\script.h
# End Source File
# Begin Source File

SOURCE=..\include\sound.h
# End Source File
# Begin Source File

SOURCE=..\include\soundserver.h
# End Source File
# Begin Source File

SOURCE=..\include\uinput.h
# End Source File
# Begin Source File

SOURCE=..\include\util.h
# End Source File
# Begin Source File

SOURCE=..\include\versions.h
# End Source File
# Begin Source File

SOURCE=..\include\vm.h
# End Source File
# Begin Source File

SOURCE=..\include\vocabulary.h
# End Source File
# End Group
# Begin Source File

SOURCE=.\freesci.def
# End Source File
# End Target
# End Project
