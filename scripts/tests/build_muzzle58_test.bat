@echo off
set "FC3ROOT=%~dp0..\..\"
setlocal
call "%~dp0..\vsenv32.bat" || exit /b 1
if errorlevel 1 exit /b 1
cl /nologo /W4 /O2 /MT /EHsc /I"%FC3ROOT%third_party\minhook\include" /Fo"%FC3ROOT%build\stereo\muzzle58_fixture.obj" "%FC3ROOT%tests\muzzle58_fixture.cpp" /link /OUT:"%FC3ROOT%build\stereo\muzzle58_fixture.exe"
if errorlevel 1 exit /b 1
"%FC3ROOT%build\stereo\muzzle58_fixture.exe"
exit /b %errorlevel%
