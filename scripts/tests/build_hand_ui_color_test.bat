@echo off
set "FC3ROOT=%~dp0..\..\"
call "%~dp0..\vsenv32.bat" || exit /b 1
cl /nologo /O2 /MT /EHsc /Fo"%FC3ROOT%build\stereo\hand_ui_color_test.obj" "%FC3ROOT%tests\hand_ui_color_test.cpp" /link d3d11.lib d3dcompiler.lib /OUT:"%FC3ROOT%build\stereo\hand_ui_color_test.exe"
if errorlevel 1 exit /b 1
"%FC3ROOT%build\stereo\hand_ui_color_test.exe"
