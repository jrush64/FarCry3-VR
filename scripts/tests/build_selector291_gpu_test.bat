@echo off
set "FC3ROOT=%~dp0..\..\"
call "%~dp0..\vsenv32.bat" || exit /b 1
cl /I"%FC3ROOT%third_party\imgui" /nologo /O2 /MT /EHsc /I"%FC3ROOT%third_party\openxr\include" /Fo"%FC3ROOT%build\stereo\selector291_gpu_test.obj" "%FC3ROOT%tests\selector291_gpu_test.cpp" /link d3d11.lib d3dcompiler.lib user32.lib gdi32.lib /OUT:"%FC3ROOT%build\stereo\selector291_gpu_test.exe"
if errorlevel 1 exit /b 1
"%FC3ROOT%build\stereo\selector291_gpu_test.exe"
exit /b %errorlevel%
