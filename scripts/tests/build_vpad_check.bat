@echo off
set "FC3ROOT=%~dp0..\..\"
setlocal
call "%~dp0..\vsenv32.bat" || exit /b 1
set "VG=%FC3ROOT%third_party\vigemclient"
cl /I"%FC3ROOT%third_party\imgui" /nologo /O2 /MT /EHsc /W0 /I"%VG%\include" /I"%VG%\src" /Fo"%FC3ROOT%build\padtest\\" "%FC3ROOT%tests\vpad_check.cpp" "%VG%\src\ViGEmClient.cpp" /link setupapi.lib xinput.lib /OUT:"%FC3ROOT%build\padtest\vpad_check.exe"
