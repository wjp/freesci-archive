@echo off
rem
rem Visual C++ Application Profiler Batch File
rem By Alexander Angas <wgd@adelaide.on.net>, 2001.
rem (Based on VC++ 6.0.)

rem
rem How to use:
rem 0. Set your options below!
rem 1. Tick 'Enable profiling' in Project Settings of project.
rem 2. Rebuild project.
rem 3. Go to Build / Profile...
rem 4. Select custom and locate this batch file.
rem 5. Click OK.

rem
rem Notes:
rem * It is recommended to close all other open applications before
rem   commencing a profile.
rem
rem * Read the MSDN documentation for more information on Win32 profiling.
rem
rem * This batch file is free for anyone to copy anywhere, as long as my
rem   name and e-mail address is present at the top.

rem
rem SET YOUR OPTIONS:

rem
rem Select which profiling to do.
rem Options: FUNCTION_TIMING, FUNCTION_COUNTING, FUNCTION_COVERAGE,
rem          LINE_COUNTING, LINE_COVERAGE
set PROFILE_CHOICE=FUNCTION_TIMING

rem
rem Change to directory containing executable.
cd ..\..\..\bin

rem
rem Set executable extension of profile.
set PROFILE_EXT=_profile.txt

rem
rem Set further options for prep parse 1 (e.g. /excall /inc ...).
rem See MSDN documentation for the PREP command for more about this.
set FURTHER_P1_OPTS=

rem
rem Set further options for plist (e.g. /t ...):
rem See MSDN documentation for the PLIST command for more about this.
set FURTHER_PLIST_OPTS=

rem
rem -----------------------------------------------------------------------


rem
rem START PROCESSING
goto %PROFILE_CHOICE%

:FUNCTION_TIMING
echo Profiling function timing in %~1%.exe...
set PROFILE_FILE=%~1_ftime%PROFILE_EXT%
set PREP1_OPTS=/NOLOGO /AT /STACK 6 /OM /FT %FURTHER_P1_OPTS% %1
set PLIST_OPTS=/NOLOGO /TAB 2 %FURTHER_PLIST_OPTS% /SC %1
goto PROCESS

:FUNCTION_COUNTING
echo Profiling function counting in %~1%.exe...
set PROFILE_FILE=%~1_fcount%PROFILE_EXT%
set PREP1_OPTS=/NOLOGO /AT /STACK 6 /OM /FC %FURTHER_P1_OPTS% %1
set PLIST_OPTS=/NOLOGO /TAB 2 %FURTHER_PLIST_OPTS% /SC %1
goto PROCESS

:FUNCTION_COVERAGE
echo Profiling function coverage in %~1%.exe...
set PROFILE_FILE=%~1_fcover%PROFILE_EXT%
set PREP1_OPTS=/NOLOGO /AT /STACK 6 /OM /FV %FURTHER_P1_OPTS% %1
set PLIST_OPTS=/NOLOGO /TAB 2 %FURTHER_PLIST_OPTS% /SC %1
goto PROCESS

:LINE_COUNTING
echo Profiling line counting in %~1%.exe...
set PROFILE_FILE=%~1_lcount%PROFILE_EXT%
set PREP1_OPTS=/NOLOGO /OM /LC %FURTHER_P1_OPTS% %1
set PLIST_OPTS=/NOLOGO /TAB 2 %FURTHER_PLIST_OPTS% /SC %1
goto PROCESS

:LINE_COVERAGE
echo Profiling line coverage in %~1%.exe...
set PROFILE_FILE=%~1_lcover%PROFILE_EXT%
set PREP1_OPTS=/NOLOGO /OM /LV %FURTHER_P1_OPTS% %1
set PLIST_OPTS=/NOLOGO /TAB 2 %FURTHER_PLIST_OPTS% /SC %1
goto PROCESS

:PROCESS
PREP %PREP1_OPTS%
if errorlevel == 1 goto ERROR
PROFILE /NOLOGO %1 %2 %3 %4 %5 %6 %7 %8 %9
if errorlevel == 1 goto ERROR
PREP /NOLOGO /M %1
if errorlevel == 1 goto ERROR
PLIST %PLIST_OPTS% >%PROFILE_FILE%
goto done

:ERROR
echo.
echo Profile not completed - an ERROR occurred.
goto end

:done
echo.
echo ***
echo Profile results saved to:
echo %PROFILE_FILE%

:end
