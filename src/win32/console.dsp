# Microsoft Developer Studio Project File - Name="console" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Console Application" 0x0103

CFG=console - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "console.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "console.mak" CFG="console - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "console - Win32 Release" (based on "Win32 (x86) Console Application")
!MESSAGE "console - Win32 Debug" (based on "Win32 (x86) Console Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "console - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "console_Release"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /D "_MBCS" /YX /FD /c
# ADD CPP /nologo /W3 /GX /O2 /I "..\include" /I "..\..\..\glib" /I "\cygnus\cygwin-b20\src" /I "\cygnus\cygwin-b20\src\include" /D "NDEBUG" /D "WIN32" /D "_CONSOLE" /D "_MBCS" /D PACKAGE=\"freesci\" /D VERSION=\"0.2.5\" /YX /FD /c
# ADD BASE RSC /l 0x419 /d "NDEBUG"
# ADD RSC /l 0x419 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /machine:I386

!ELSEIF  "$(CFG)" == "console - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "console___Win32_Debug"
# PROP BASE Intermediate_Dir "console___Win32_Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "console_Debug"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /D "_MBCS" /YX /FD /GZ /c
# ADD CPP /nologo /W3 /Gm /GX /ZI /Od /I "..\include" /I "..\..\..\glib" /I "\cygnus\cygwin-b20\src" /I "\cygnus\cygwin-b20\src\include" /D "_DEBUG" /D "WIN32" /D "_CONSOLE" /D "_MBCS" /D PACKAGE=\"freesci\" /D VERSION=\"0.2.5\" /YX /FD /GZ /c
# ADD BASE RSC /l 0x419 /d "_DEBUG"
# ADD RSC /l 0x419 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /debug /machine:I386 /pdbtype:sept
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /debug /machine:I386 /pdbtype:sept

!ENDIF 

# Begin Target

# Name "console - Win32 Release"
# Name "console - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=..\console\commands.c
# End Source File
# Begin Source File

SOURCE=..\console\commands_scr.c
# End Source File
# Begin Source File

SOURCE=..\console\console.c
# End Source File
# Begin Source File

SOURCE=..\console\console_font.c
# End Source File
# Begin Source File

SOURCE=..\core\decompress0.c
# End Source File
# Begin Source File

SOURCE=..\core\decompress1.c
# End Source File
# Begin Source File

SOURCE=..\core\resource.c
# End Source File
# Begin Source File

SOURCE=..\tools\sciconsole.c
# End Source File
# Begin Source File

SOURCE=..\core\script.c
# End Source File
# Begin Source File

SOURCE=..\sound\sound.c
# End Source File
# Begin Source File

SOURCE=..\core\vocabulary.c
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE="..\..\..\..\cygnus\cygwin-b20\src\readline\chardefs.h"
# End Source File
# Begin Source File

SOURCE=..\include\config.h
# End Source File
# Begin Source File

SOURCE=..\include\console.h
# End Source File
# Begin Source File

SOURCE=..\include\engine.h
# End Source File
# Begin Source File

SOURCE=..\..\..\glib\glib.h
# End Source File
# Begin Source File

SOURCE=..\..\..\glib\glibconfig.h
# End Source File
# Begin Source File

SOURCE=..\include\graphics.h
# End Source File
# Begin Source File

SOURCE=..\include\heap.h
# End Source File
# Begin Source File

SOURCE="..\..\..\..\cygnus\cygwin-b20\src\readline\keymaps.h"
# End Source File
# Begin Source File

SOURCE=..\include\menubar.h
# End Source File
# Begin Source File

SOURCE="..\..\..\..\cygnus\cygwin-b20\src\include\obstack.h"
# End Source File
# Begin Source File

SOURCE=..\include\resource.h
# End Source File
# Begin Source File

SOURCE=..\include\script.h
# End Source File
# Begin Source File

SOURCE=..\include\sound.h
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

SOURCE="..\..\..\..\cygnus\cygwin-b20\src\readline\history.h"
# End Source File
# Begin Source File

SOURCE="..\..\..\..\cygnus\cygwin-b20\src\readline\readline.h"
# End Source File
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

SOURCE="..\..\..\glib\glib-1.3.lib"
# End Source File
# End Target
# End Project
