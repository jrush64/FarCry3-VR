@echo off
set "FC3ROOT=%~dp0..\..\"
setlocal

call "%~dp0..\vsenv32.bat" || exit /b 1


if errorlevel 1 exit /b 1

if not exist "%FC3ROOT%build" mkdir "%FC3ROOT%build"

cl /I"%FC3ROOT%third_party\imgui" /nologo /W4 /O2 /MT /LD /Fo"%FC3ROOT%build\proxy.obj" "%FC3ROOT%src\proxy.cpp" /link /DEF:"%FC3ROOT%src\d3d11.def" /OUT:"%FC3ROOT%build\d3d11.dll" /IMPLIB:"%FC3ROOT%build\d3d11.lib"
if errorlevel 1 exit /b 1

cl /I"%FC3ROOT%third_party\imgui" /nologo /W4 /O2 /MT /Fo"%FC3ROOT%build\smoke.obj" "%FC3ROOT%tests\smoke.cpp" /link /OUT:"%FC3ROOT%build\smoke.exe"
if errorlevel 1 exit /b 1

"%FC3ROOT%build\smoke.exe" "%FC3ROOT%build\d3d11.dll"
exit /b %errorlevel%
