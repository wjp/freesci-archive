# Microsoft Developer Studio Project File - Name="sciv" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Console Application" 0x0103

CFG=sciv - Win32 Purify
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "sciv.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "sciv.mak" CFG="sciv - Win32 Purify"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "sciv - Win32 Release" (based on "Win32 (x86) Console Application")
!MESSAGE "sciv - Win32 Debug" (based on "Win32 (x86) Console Application")
!MESSAGE "sciv - Win32 Purify" (based on "Win32 (x86) Console Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "sciv - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "sciv_Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /D "_MBCS" /YX /FD /c
# ADD CPP /nologo /MD /W3 /Gi- /vmb /vd1 /GR- /GX /Zi /O1 /Ob2 /Gf /Gy /I "..\..\..\SDL\include" /I "..\include" /I "..\include\win32" /D "NDEBUG" /D PACKAGE=\"FreeSCI\" /D VERSION=\"0.3.4\" /D "WIN32" /D "_CONSOLE" /D "HAVE_SDL" /D "HAVE_GETOPT_H" /D "HAVE_USLEEP" /D "HAVE_SYS_STAT_H" /D "HAVE_FCNTL_H" /D "HAVE_STRING_H" /FR /YX /FD /D /c
# ADD BASE RSC /l 0x419 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /machine:I386
# ADD LINK32 Release\fsci.lib winmm.lib kernel32.lib user32.lib gdi32.lib advapi32.lib uuid.lib /nologo /subsystem:console /debug /machine:I386 /out:"..\..\bin\freesci.exe" /fixed:no
# SUBTRACT LINK32 /pdb:none

!ELSEIF  "$(CFG)" == "sciv - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "sciv___Win32_Debug"
# PROP BASE Intermediate_Dir "sciv___Win32_Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "sciv_Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /D "_MBCS" /YX /FD /GZ /c
# ADD CPP /nologo /MDd /W3 /GR- /GX /Zi /Od /I "..\..\..\SDL\include" /I "..\include" /I "..\include\win32" /D "_DEBUG" /D PACKAGE=\"freesci\" /D VERSION=__TIMESTAMP__ /D "HAVE_GETOPT_H" /D "HAVE_LIBPNG" /D "WIN32" /D "_CONSOLE" /D "_MBCS" /D "HAVE_STRING_H" /D "HAVE_SYS_STAT_H" /D "HAVE_FCNTL_H" /FR /YX /FD /D /GZ /c
# ADD BASE RSC /l 0x419 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /debug /machine:I386 /pdbtype:sept
# ADD LINK32 Debug\fsci.lib winmm.lib kernel32.lib user32.lib gdi32.lib advapi32.lib uuid.lib /nologo /subsystem:console /incremental:no /debug /machine:I386 /out:"Debug\freesci.exe" /pdbtype:sept /fixed:no
# SUBTRACT LINK32 /pdb:none

!ELSEIF  "$(CFG)" == "sciv - Win32 Purify"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "sciv___Win32_Purify"
# PROP BASE Intermediate_Dir "sciv___Win32_Purify"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Purify"
# PROP Intermediate_Dir "sciv_Purify"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MDd /W3 /Gm /Gi /GR /GX /ZI /Od /I "..\..\..\SDL\include" /I "..\include" /I "..\include\win32" /D PACKAGE=\"freesci\" /D "HAVE_GETOPT_H" /D "HAVE_LIBPNG" /D VERSION=__TIMESTAMP__ /D "_DEBUG" /D "WIN32" /D "_CONSOLE" /D "_MBCS" /D "HAVE_STRING_H" /D "SATISFY_PURIFY" /FR /YX /FD /GZ /c
# ADD CPP /nologo /MT /W3 /GR /GX /Zi /Od /I "..\..\..\SDL\include" /I "..\include" /I "..\include\win32" /D "SATISFY_PURIFY" /D "_DEBUG" /D PACKAGE=\"freesci\" /D VERSION=__TIMESTAMP__ /D "HAVE_GETOPT_H" /D "HAVE_LIBPNG" /D "WIN32" /D "_CONSOLE" /D "_MBCS" /D "HAVE_STRING_H" /D "HAVE_SYS_STAT_H" /D "HAVE_FCNTL_H" /FR /YX /FD /GZ /c
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 msvcrtd.lib Purify\fsci.lib winmm.lib kernel32.lib user32.lib gdi32.lib advapi32.lib uuid.lib /nologo /subsystem:console /map /debug /machine:I386 /nodefaultlib:"libc.lib" /nodefaultlib:"libcmt.lib" /nodefaultlib:"msvcrt.lib" /nodefaultlib:"libcd.lib" /nodefaultlib:"libcmtd.lib" /out:"Debug\freesci.exe"
# SUBTRACT BASE LINK32 /pdb:none
# ADD LINK32 Purify\fsci.lib winmm.lib kernel32.lib user32.lib gdi32.lib advapi32.lib uuid.lib /nologo /subsystem:console /incremental:no /map:"Purify/freesci.map" /debug /machine:I386 /out:"Purify\freesci.exe" /fixed:no
# SUBTRACT LINK32 /pdb:none

!ENDIF 

# Begin Target

# Name "sciv - Win32 Release"
# Name "sciv - Win32 Debug"
# Name "sciv - Win32 Purify"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=..\config.c
# End Source File
# Begin Source File

SOURCE=..\config.l
# End Source File
# Begin Source File

SOURCE=.\getopt.c
# End Source File
# Begin Source File

SOURCE=..\main.c
# End Source File
# Begin Source File

SOURCE=..\scicore\sci_memory.c
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter ""
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

SOURCE=..\include\games.h
# End Source File
# Begin Source File

SOURCE=..\include\win32\getopt.h
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

SOURCE=..\include\graphics.h
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

SOURCE=..\include\resource.h
# End Source File
# Begin Source File

SOURCE=..\include\sbtree.h
# End Source File
# Begin Source File

SOURCE=..\include\sci_conf.h
# End Source File
# Begin Source File

SOURCE=..\include\sci_memory.h
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
# End Target
# End Project
