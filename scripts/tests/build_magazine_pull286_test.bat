@echo off
set "FC3ROOT=%~dp0..\..\"
call "%~dp0..\vsenv32.bat" || exit /b 1
if errorlevel 1 exit /b 1
cl /nologo /W4 /O2 /MT /EHsc /Fo"%FC3ROOT%build\stereo\magazine_pull286_test.obj" "%FC3ROOT%tests\magazine_pull286_test.cpp" /link /OUT:"%FC3ROOT%build\stereo\magazine_pull286_test.exe"
if errorlevel 1 exit /b 1
"%FC3ROOT%build\stereo\magazine_pull286_test.exe"
exit /b %errorlevel%
