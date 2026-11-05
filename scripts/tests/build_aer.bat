@echo off
set "FC3ROOT=%~dp0..\..\"
setlocal

call "%~dp0..\vsenv32.bat" || exit /b 1
if errorlevel 1 exit /b 1

if not exist "%FC3ROOT%build" mkdir "%FC3ROOT%build"
set "MH=%FC3ROOT%third_party\minhook"
set "MHINC=/I""%MH%\include"" /I""%MH%\src"" /I""%MH%\src\hde"""

cl /I"%FC3ROOT%third_party\imgui" /nologo /W4 /O2 /MT %MHINC% /c /Fo"%FC3ROOT%build\aer_proxy.obj" "%FC3ROOT%src\aer_proxy.cpp"
if errorlevel 1 exit /b 1
cl /I"%FC3ROOT%third_party\imgui" /nologo /W4 /O2 /MT %MHINC% /c /Fo"%FC3ROOT%build\aer_state.obj" "%FC3ROOT%src\aer_state.cpp"
if errorlevel 1 exit /b 1
cl /I"%FC3ROOT%third_party\imgui" /nologo /W4 /O2 /MT %MHINC% /c /Fo"%FC3ROOT%build\aer_camera.obj" "%FC3ROOT%src\aer_camera.cpp"
if errorlevel 1 exit /b 1
cl /I"%FC3ROOT%third_party\imgui" /nologo /W4 /O2 /MT /TC %MHINC% /c /Fo"%FC3ROOT%build\mh_hook.obj" "%MH%\src\hook.c"
if errorlevel 1 exit /b 1
cl /I"%FC3ROOT%third_party\imgui" /nologo /W4 /O2 /MT /TC %MHINC% /c /Fo"%FC3ROOT%build\mh_buffer.obj" "%MH%\src\buffer.c"
if errorlevel 1 exit /b 1
cl /I"%FC3ROOT%third_party\imgui" /nologo /W4 /O2 /MT /TC %MHINC% /c /Fo"%FC3ROOT%build\mh_trampoline.obj" "%MH%\src\trampoline.c"
if errorlevel 1 exit /b 1
cl /I"%FC3ROOT%third_party\imgui" /nologo /W4 /O2 /MT /TC %MHINC% /c /Fo"%FC3ROOT%build\mh_hde32.obj" "%MH%\src\hde\hde32.c"
if errorlevel 1 exit /b 1
link /nologo /DLL "%FC3ROOT%build\aer_proxy.obj" "%FC3ROOT%build\aer_state.obj" "%FC3ROOT%build\aer_camera.obj" "%FC3ROOT%build\mh_hook.obj" "%FC3ROOT%build\mh_buffer.obj" "%FC3ROOT%build\mh_trampoline.obj" "%FC3ROOT%build\mh_hde32.obj" /DEF:"%FC3ROOT%src\d3d11.def" /OUT:"%FC3ROOT%build\d3d11.dll" /IMPLIB:"%FC3ROOT%build\d3d11.lib"
if errorlevel 1 exit /b 1

cl /I"%FC3ROOT%third_party\imgui" /nologo /W4 /O2 /MT /Fo"%FC3ROOT%build\aer_smoke.obj" "%FC3ROOT%tests\aer_smoke.cpp" /link user32.lib /OUT:"%FC3ROOT%build\aer_smoke.exe"
if errorlevel 1 exit /b 1
"%FC3ROOT%build\aer_smoke.exe" "%FC3ROOT%build\d3d11.dll"
exit /b %errorlevel%
