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
# ADD CPP /nologo /MD /W3 /GX /O2 /I "..\include" /I "..\..\..\glib" /I "\cygnus\cygwin-b20\src" /I "\cygnus\cygwin-b20\src\include" /I "..\..\..\hermes\src" /I "..\..\..\libpng" /I "..\..\..\zlib" /D "NDEBUG" /D "WIN32" /D "_CONSOLE" /D "_MBCS" /D PACKAGE=\"freesci\" /D VERSION=\"0.2.6\" /D "HAVE_DDRAW" /D "HAVE_STRING_H" /D "HAVE_OBSTACK_H" /D "HAVE_GETOPT_H" /D "HAVE_READLINE_READLINE_H" /D "HAVE_READLINE_HISTORY_H" /D "HAVE_LIBPNG" /D "FREESCI_EXPORTS" /YX /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x419 /d "NDEBUG"
# ADD RSC /l 0x419 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib ddraw.lib winmm.lib /nologo /dll /machine:I386 /nodefaultlib:"libc.lib"

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
# ADD CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /I "..\include" /I "..\..\..\glib" /I "\cygnus\cygwin-b20\src" /I "\cygnus\cygwin-b20\src\include" /I "..\..\..\hermes\src" /I "..\..\..\libpng" /I "..\..\..\zlib" /D "_DEBUG" /D "WIN32" /D "_CONSOLE" /D "_MBCS" /D PACKAGE=\"freesci\" /D VERSION=\"0.2.6\" /D "HAVE_DDRAW" /D "HAVE_STRING_H" /D "HAVE_OBSTACK_H" /D "HAVE_GETOPT_H" /D "HAVE_READLINE_READLINE_H" /D "HAVE_READLINE_HISTORY_H" /D "HAVE_LIBPNG" /D "FREESCI_EXPORTS" /YX /FD /GZ /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x419 /d "_DEBUG"
# ADD RSC /l 0x419 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /debug /machine:I386 /pdbtype:sept
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib ddraw.lib winmm.lib /nologo /dll /debug /machine:I386 /nodefaultlib:"LIBC" /pdbtype:sept

!ENDIF 

# Begin Target

# Name "freesci - Win32 Release"
# Name "freesci - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=..\engine\cfsml.pl

!IF  "$(CFG)" == "freesci - Win32 Release"

# Begin Custom Build
InputDir=\Src\freesci\src\engine
InputPath=..\engine\cfsml.pl
InputName=cfsml

"..\engine\savegame.c" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	cd $(InputDir) 
	d:\perl\bin\perl cfsml.pl < $(InputName).cfsml > $(InputName).c 
	
# End Custom Build

!ELSEIF  "$(CFG)" == "freesci - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\graphics\con_font.c
# End Source File
# Begin Source File

SOURCE=..\graphics\con_io.c
# End Source File
# Begin Source File

SOURCE=..\scicore\console.c
# End Source File
# Begin Source File

SOURCE=..\scicore\decompress0.c
# End Source File
# Begin Source File

SOURCE=..\scicore\decompress1.c
# End Source File
# Begin Source File

SOURCE=..\graphics\engine_graphics.c
# End Source File
# Begin Source File

SOURCE=..\graphics\font.c
# End Source File
# Begin Source File

SOURCE=..\engine\game.c
# End Source File
# Begin Source File

SOURCE=..\engine\grammar.c
# End Source File
# Begin Source File

SOURCE=..\graphics\graphics.c
# End Source File
# Begin Source File

SOURCE=..\graphics\graphics_ddraw.c
# End Source File
# Begin Source File

SOURCE=..\graphics\graphics_ggi.c
# End Source File
# Begin Source File

SOURCE=..\graphics\graphics_png.c
# End Source File
# Begin Source File

SOURCE=..\engine\heap.c
# End Source File
# Begin Source File

SOURCE=..\graphics\input.c
# End Source File
# Begin Source File

SOURCE=..\graphics\input_ggi.c
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

SOURCE=..\graphics\mcursor.c
# End Source File
# Begin Source File

SOURCE=..\graphics\menubar.c
# End Source File
# Begin Source File

SOURCE=..\sound\midi.c
# End Source File
# Begin Source File

SOURCE=..\scicore\old_objects.c
# End Source File
# Begin Source File

SOURCE=..\scicore\resource.c
# End Source File
# Begin Source File

SOURCE=..\engine\said.c
# End Source File
# Begin Source File

SOURCE=..\engine\savegame.c
# End Source File
# Begin Source File

SOURCE=..\engine\savegame.cfsml

!IF  "$(CFG)" == "freesci - Win32 Release"

# Begin Custom Build
InputDir=\Src\freesci\src\engine
InputPath=..\engine\savegame.cfsml
InputName=savegame

"..\engine\savegame.c" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	cd $(InputDir) 
	d:\perl\bin\perl cfsml.pl < $(InputName).cfsml > $(InputName).c 
	
# End Custom Build

!ELSEIF  "$(CFG)" == "freesci - Win32 Debug"

# Begin Custom Build
InputDir=\Src\freesci\src\engine
InputPath=..\engine\savegame.cfsml
InputName=savegame

"..\engine\savegame.c" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	cd $(InputDir) 
	d:\perl\bin\perl cfsml.pl < $(InputName).cfsml > $(InputName).c 
	
# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\scicore\script.c
# End Source File
# Begin Source File

SOURCE=..\engine\scriptdebug.c
# End Source File
# Begin Source File

SOURCE=..\engine\simplesaid.c
# End Source File
# Begin Source File

SOURCE=..\sound\sound.c
# End Source File
# Begin Source File

SOURCE=..\engine\state.c
# End Source File
# Begin Source File

SOURCE=..\scicore\tools.c
# End Source File
# Begin Source File

SOURCE=..\engine\vm.c
# End Source File
# Begin Source File

SOURCE=..\scicore\vocab.c
# End Source File
# Begin Source File

SOURCE=..\scicore\vocab_debug.c
# End Source File
# Begin Source File

SOURCE=..\graphics\window.c
# End Source File
# End Group
# Begin Group "Libs"

# PROP Default_Filter ""
# Begin Group "readline"

# PROP Default_Filter ""
# Begin Group "Source"

# PROP Default_Filter ""
# Begin Source File

SOURCE="..\..\..\..\cygnus\cygwin-b20\src\readline\bind.c"
# End Source File
# Begin Source File

SOURCE="..\..\..\..\cygnus\cygwin-b20\src\readline\complete.c"
# End Source File
# Begin Source File

SOURCE="..\..\..\..\cygnus\cygwin-b20\src\readline\display.c"
# End Source File
# Begin Source File

SOURCE="..\..\..\..\cygnus\cygwin-b20\src\readline\funmap.c"
# End Source File
# Begin Source File

SOURCE="..\..\..\..\cygnus\cygwin-b20\src\readline\history.c"
# End Source File
# Begin Source File

SOURCE="..\..\..\..\cygnus\cygwin-b20\src\readline\isearch.c"
# End Source File
# Begin Source File

SOURCE="..\..\..\..\cygnus\cygwin-b20\src\readline\keymaps.c"
# End Source File
# Begin Source File

SOURCE="..\..\..\..\cygnus\cygwin-b20\src\readline\parens.c"
# End Source File
# Begin Source File

SOURCE="..\..\..\..\cygnus\cygwin-b20\src\readline\readline.c"
# End Source File
# Begin Source File

SOURCE="..\..\..\..\cygnus\cygwin-b20\src\readline\rltty.c"
# End Source File
# Begin Source File

SOURCE="..\..\..\..\cygnus\cygwin-b20\src\readline\search.c"
# End Source File
# Begin Source File

SOURCE="..\..\..\..\cygnus\cygwin-b20\src\readline\tilde.c"
# End Source File
# Begin Source File

SOURCE="..\..\..\..\cygnus\cygwin-b20\src\readline\xmalloc.c"
# End Source File
# End Group
# Begin Group "Header"

# PROP Default_Filter ""
# Begin Source File

SOURCE="..\..\..\..\cygnus\cygwin-b20\src\readline\rldefs.h"
# End Source File
# Begin Source File

SOURCE="..\..\..\..\cygnus\cygwin-b20\src\readline\sysdep.h"
# End Source File
# Begin Source File

SOURCE="..\..\..\..\cygnus\cygwin-b20\src\readline\tilde.h"
# End Source File
# End Group
# End Group
# Begin Source File

SOURCE=D:\VStudio\VC98\Include\BASETSD.H
# End Source File
# Begin Source File

SOURCE="..\..\..\..\cygnus\cygwin-b20\src\readline\chardefs.h"
# End Source File
# Begin Source File

SOURCE=..\..\..\glib\config.h
# End Source File
# Begin Source File

SOURCE="..\..\..\..\cygnus\cygwin-b20\src\readline\emacs_keymap.c"
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=..\include\event.h
# End Source File
# Begin Source File

SOURCE="..\..\..\..\cygnus\cygwin-b20\src\libiberty\getopt.c"
# End Source File
# Begin Source File

SOURCE="..\..\..\..\cygnus\cygwin-b20\src\include\getopt.h"
# End Source File
# Begin Source File

SOURCE="..\..\..\..\cygnus\cygwin-b20\src\libiberty\getopt1.c"
# End Source File
# Begin Source File

SOURCE=..\..\..\glib\glib.h
# End Source File
# Begin Source File

SOURCE=..\..\..\glib\glibconfig.h
# End Source File
# Begin Source File

SOURCE=..\..\..\Hermes\src\H_Clear.h
# End Source File
# Begin Source File

SOURCE=..\..\..\Hermes\src\H_Config.h
# End Source File
# Begin Source File

SOURCE=..\..\..\Hermes\src\H_Conv.h
# End Source File
# Begin Source File

SOURCE=..\..\..\Hermes\src\H_Format.h
# End Source File
# Begin Source File

SOURCE=..\..\..\Hermes\src\H_Pal.h
# End Source File
# Begin Source File

SOURCE=..\..\..\Hermes\src\H_Types.h
# End Source File
# Begin Source File

SOURCE=..\..\..\Hermes\src\Hermes.h
# End Source File
# Begin Source File

SOURCE="..\..\..\..\cygnus\cygwin-b20\src\readline\history.h"
# End Source File
# Begin Source File

SOURCE="..\..\..\..\cygnus\cygwin-b20\src\readline\keymaps.h"
# End Source File
# Begin Source File

SOURCE="..\..\..\..\cygnus\cygwin-b20\src\libiberty\obstack.c"
# End Source File
# Begin Source File

SOURCE="..\..\..\..\cygnus\cygwin-b20\src\include\obstack.h"
# End Source File
# Begin Source File

SOURCE=..\..\..\libpng\png.h
# End Source File
# Begin Source File

SOURCE=..\..\..\libpng\pngconf.h
# End Source File
# Begin Source File

SOURCE="..\..\..\..\cygnus\cygwin-b20\src\readline\readline.h"
# End Source File
# Begin Source File

SOURCE=..\..\..\zlib\zconf.h
# End Source File
# Begin Source File

SOURCE=..\..\..\zlib\zlib.h
# End Source File
# Begin Source File

SOURCE="..\..\..\glib\glib-1.3.lib"
# End Source File
# Begin Source File

SOURCE=..\..\..\Hermes\src\Release\Hermes.lib
# End Source File
# Begin Source File

SOURCE=..\..\..\libpng\libpng.lib
# End Source File
# Begin Source File

SOURCE=..\..\..\zlib\zlib.lib
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

SOURCE=..\include\graphics.h
# End Source File
# Begin Source File

SOURCE=..\include\graphics_ddraw.h
# End Source File
# Begin Source File

SOURCE=..\include\graphics_ggi.h
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

SOURCE=..\include\old_objects.h
# End Source File
# Begin Source File

SOURCE=..\include\resource.h
# End Source File
# Begin Source File

SOURCE=..\include\sci_conf.h
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
