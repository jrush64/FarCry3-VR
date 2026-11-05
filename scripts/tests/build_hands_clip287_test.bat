@echo off
set "FC3ROOT=%~dp0..\..\"
call "%~dp0..\vsenv32.bat" || exit /b 1
if errorlevel 1 exit /b 1
cl /nologo /W4 /O2 /MT /EHsc /Fo"%FC3ROOT%build\stereo\hands_clip287_test.obj" "%FC3ROOT%tests\hands_clip287_test.cpp" /link d3dcompiler.lib user32.lib /OUT:"%FC3ROOT%build\stereo\hands_clip287_test.exe"
if errorlevel 1 exit /b 1
"%FC3ROOT%build\stereo\hands_clip287_test.exe"
exit /b %errorlevel%
