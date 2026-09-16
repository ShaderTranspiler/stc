@echo off

REM This script is shipped as a release artifact

REM Runs the stc CLI with the proper Julia libraries reachable for loading
REM For v1.0 these wrapper scripts are planned to be replaced by a binary launcher wrapper instead
REM That would result on a single executable instead of wrapper scripts, but requires a bit of work

REM define STC_JULIA_LIBDIR in the environment to avoid a dynamic lookup through julia executable each launch

setlocal

if defined STC_JULIA_LIBDIR (
    set "JULIA_LIBDIR=%STC_JULIA_LIBDIR%"
) else (
    where julia >nul 2>&1
    if errorlevel 1 (
        echo stc: 'julia' is not reachable from PATH. Install Julia, or set STC_JULIA_LIBDIR to the directory containing libjulia manually. 1>&2
        exit /b 1
    )

    for /f "delims=" %%i in ('julia -e "print(Sys.BINDIR)"') do set "JULIA_LIBDIR=%%i"
)

set "PATH=%JULIA_LIBDIR%;%PATH%"

REM this language is truly something else
REM this runs stc.exe in the script's directory with all args forwarded
"%~dp0stc.exe" %*

exit /b %errorlevel%
