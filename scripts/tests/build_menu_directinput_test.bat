@echo off
set "FC3ROOT=%~dp0..\..\"
call "%~dp0..\vsenv32.bat" || exit /b 1
cl /nologo /O2 /MT /EHsc /I"%FC3ROOT%third_party\minhook\include" /Fo"%FC3ROOT%build\stereo\menu_directinput_test.obj" "%FC3ROOT%tests\menu_directinput_test.cpp" /link user32.lib dxguid.lib /OUT:"%FC3ROOT%build\stereo\menu_directinput_test.exe"
if errorlevel 1 exit /b 1
"%FC3ROOT%build\stereo\menu_directinput_test.exe"
