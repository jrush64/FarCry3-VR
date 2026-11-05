@echo off
set "FC3ROOT=%~dp0..\..\"
call "%~dp0..\vsenv32.bat" || exit /b 1
cl /nologo /O2 /MT /EHsc /c /Fo"%FC3ROOT%build\stereo\build_selector292_native_visual_test.obj" "%FC3ROOT%src\dxbc_patch.cpp" || exit /b 1
cl /nologo /W4 /O2 /MT /EHsc /Fo"%FC3ROOT%build\stereo\selector292_native_visual_test.obj" "%FC3ROOT%tests\selector292_native_visual_test.cpp" /link "%FC3ROOT%build\stereo\build_selector292_native_visual_test.obj" d3d11.lib d3dcompiler.lib dxguid.lib /OUT:"%FC3ROOT%build\stereo\selector292_native_visual_test.exe"
if errorlevel 1 exit /b 1
"%FC3ROOT%build\stereo\selector292_native_visual_test.exe"
exit /b %errorlevel%
