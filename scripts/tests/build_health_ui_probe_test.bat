@echo off
set "FC3ROOT=%~dp0..\..\"
call "%~dp0..\vsenv32.bat" || exit /b 1
cl /nologo /W4 /O2 /MT /EHsc /Fo"%FC3ROOT%build\stereo\health_ui_probe_test.obj" "%FC3ROOT%tests\health_ui_probe_test.cpp" /link d3d11.lib /OUT:"%FC3ROOT%build\stereo\health_ui_probe_test.exe"
if errorlevel 1 exit /b 1
"%FC3ROOT%build\stereo\health_ui_probe_test.exe"
