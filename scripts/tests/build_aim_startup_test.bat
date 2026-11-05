@echo off
set "FC3ROOT=%~dp0..\..\"
call "%~dp0..\vsenv32.bat" || exit /b 1
cl /I"%FC3ROOT%third_party\imgui" /I"%FC3ROOT%third_party\minhook\include" /nologo /W4 /O2 /MT /EHsc /Fo"%FC3ROOT%build\stereo\aim_startup_test.obj" "%FC3ROOT%tests\aim_startup_test.cpp" /link /OUT:"%FC3ROOT%build\stereo\aim_startup_test.exe"
if errorlevel 1 exit /b 1
"%FC3ROOT%build\stereo\aim_startup_test.exe"
