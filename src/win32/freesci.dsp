# Microsoft Developer Studio Project File - Name="freesci" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=freesci - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "freesci.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "freesci.mak" CFG="freesci - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "freesci - Win32 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "freesci - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "freesci - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "freesci___Win32_Release"
# PROP BASE Intermediate_Dir "freesci___Win32_Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "freesci_Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "FREESCI_EXPORTS" /YX /FD /c
# ADD CPP /nologo /W3 /GX /Zi /I "..\..\..\SDL-1.2.0\include" /I "..\include" /I "..\include\win32" /D "FREESCI_EXPORTS" /D "X_DISPLAY_MISSING" /D VERSION=\"0.3.2\" /D "HAVE_SDL" /D "NDEBUG" /D "WIN32" /D "_CONSOLE" /D "_MBCS" /D PACKAGE=\"freesci\" /D "HAVE_STRING_H" /D "HAVE_GETOPT_H" /D "HAVE_USLEEP" /FR /YX /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x419 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /machine:I386
# ADD LINK32 wsock32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib winmm.lib ..\..\..\SDL-1.2.0\VisualC\SDL\Release\SDL.lib /nologo /dll /incremental:yes /debug /machine:I386
# SUBTRACT LINK32 /pdb:none

!ELSEIF  "$(CFG)" == "freesci - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "freesci___Win32_Debug"
# PROP BASE Intermediate_Dir "freesci___Win32_Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "freesci_Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "FREESCI_EXPORTS" /YX /FD /GZ /c
# ADD CPP /nologo /W3 /GX /I "..\include" /D "_DEBUG" /D "WIN32" /D "_CONSOLE" /D "_MBCS" /D PACKAGE=\"freesci\" /D VERSION=\"0.3.0\" /D "HAVE_DDRAW" /D "HAVE_STRING_H" /D "FREESCI_EXPORTS" /D "X_DISPLAY_MISSING" /FR /YX /FD /GZ /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x419 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /debug /machine:I386 /pdbtype:sept
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib ddraw.lib winmm.lib ..\..\..\SDL-1.2.0\VisualC\SDL\Release\SDL.lib /nologo /dll /incremental:no /machine:I386 /nodefaultlib:"LIBC" /pdbtype:sept
# SUBTRACT LINK32 /debug

!ENDIF 

# Begin Target

# Name "freesci - Win32 Release"
# Name "freesci - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Group "engine"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\engine\cfsml.pl
# End Source File
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

SOURCE=..\gfx\drivers\gfx_drivers.c
# End Source File
# Begin Source File

SOURCE=..\gfx\drivers\sdl_driver.c
# End Source File
# End Group
# Begin Source File

SOURCE=..\gfx\antialias.c
# End Source File
# Begin Source File

SOURCE=..\gfx\font.c
# End Source File
# Begin Source File

SOURCE=..\gfx\gfx_crossblit.c
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=..\gfx\gfx_line.c
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=..\gfx\gfx_pixmap_scale.c
# PROP Exclude_From_Build 1
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

SOURCE=..\sound\midi_device.c
# End Source File
# Begin Source File

SOURCE=..\sound\midi_mt32.c
# End Source File
# Begin Source File

SOURCE=..\sound\midi_mt32gm.c
# End Source File
# Begin Source File

SOURCE=..\sound\midiout.c
# End Source File
# Begin Source File

SOURCE=..\sound\midiout_win32mci.c
# End Source File
# Begin Source File

SOURCE=..\sound\oldmidi.c
# End Source File
# Begin Source File

SOURCE=..\sound\sfx_save.c
# End Source File
# Begin Source File

SOURCE=..\sound\sound.c
# End Source File
# Begin Source File

SOURCE=..\sound\soundserver.c
# End Source File
# Begin Source File

SOURCE=..\sound\soundserver_sdl.c
# End Source File
# End Group
# Begin Source File

SOURCE=.\usleep.c
# End Source File
# End Group
# Begin Group "Libs"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\include\event.h
# End Source File
# Begin Source File

SOURCE=c:\cygwin\usr\include\getopt.h
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=..\include\console.h
# End Source File
# Begin Source File

SOURCE=..\include\engine.h
# End Source File
# Begin Source File

SOURCE=..\include\gfx_driver.h
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

SOURCE=..\include\old_objects.h
# End Source File
# Begin Source File

SOURCE=..\include\resource.h
# End Source File
# Begin Source File

SOURCE=..\include\sbtree.h
# End Source File
# Begin Source File

SOURCE=..\include\sci_graphics.h
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
