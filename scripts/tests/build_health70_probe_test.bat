@echo off
set "FC3ROOT=%~dp0..\..\"
setlocal
call "%~dp0..\vsenv32.bat" || exit /b 1
if errorlevel 1 exit /b 1
if not exist "%FC3ROOT%build\health70" mkdir "%FC3ROOT%build\health70"
cl /nologo /W4 /O2 /MT /EHsc /Fo"%FC3ROOT%build\health70\probe.obj" "%FC3ROOT%candidates\health70\health_ui_probe70_test.cpp" /link d3d11.lib /OUT:"%FC3ROOT%build\health70\probe.exe"
if errorlevel 1 exit /b 1
"%FC3ROOT%build\health70\probe.exe"
exit /b %errorlevel%
