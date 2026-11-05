@echo off
REM Shared Visual Studio setup for all build scripts: finds VS 2022 (any edition) via vswhere
REM and enters the x86 (32-bit) developer environment. Far Cry 3 is a 32-bit game.
if defined FC3VR_VSENV_DONE exit /b 0
set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
if not exist "%VSWHERE%" set "VSWHERE=%ProgramFiles%\Microsoft Visual Studio\Installer\vswhere.exe"
if not exist "%VSWHERE%" ( echo [build] vswhere.exe not found - is Visual Studio 2022 installed? & exit /b 1 )
for /f "usebackq tokens=*" %%I in (`"%VSWHERE%" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do set "VSROOT=%%I"
if not defined VSROOT ( echo [build] no VS install with the C++ toolset found & exit /b 1 )
call "%VSROOT%\VC\Auxiliary\Build\vcvars32.bat" >nul
if errorlevel 1 ( echo [build] vcvars32 failed & exit /b 1 )
set "FC3VR_VSENV_DONE=1"
exit /b 0
