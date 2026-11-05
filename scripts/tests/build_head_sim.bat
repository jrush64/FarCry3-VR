@echo off
set "FC3ROOT=%~dp0..\..\"
setlocal
call "%~dp0..\vsenv32.bat" || exit /b 1
if errorlevel 1 exit /b 1
set "MH=%FC3ROOT%third_party\minhook"
cl /I"%FC3ROOT%third_party\imgui" /nologo /W4 /O2 /MT /EHsc /I"%MH%\include" /Fo"%FC3ROOT%build\\" "%FC3ROOT%tests\head_sim.cpp" "%FC3ROOT%src\head_track.cpp" "%MH%\src\hook.c" "%MH%\src\buffer.c" "%MH%\src\trampoline.c" "%MH%\src\hde\hde32.c" /link dinput8.lib dxguid.lib user32.lib /OUT:"%FC3ROOT%build\head_sim.exe"
if errorlevel 1 exit /b 1
"%FC3ROOT%build\head_sim.exe"
exit /b %errorlevel%
