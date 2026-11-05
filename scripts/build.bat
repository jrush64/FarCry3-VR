@echo off
set "FC3ROOT=%~dp0..\"
setlocal

call "%~dp0vsenv32.bat" || exit /b 1
if errorlevel 1 exit /b 1

for %%I in ("%FC3ROOT%.") do set "FC3ABS=%%~fI"
set "OUT=%FC3ROOT%build\stereo"
if not exist "%OUT%" mkdir "%OUT%"
set "MH=%FC3ROOT%third_party\minhook"
set "MHINC=/I"%MH%\include" /I"%MH%\src" /I"%MH%\src\hde""
set "CXX=cl "/d1trimfile:%FC3ROOT%\" "/d1trimfile:%FC3ABS%\\" /I"%FC3ROOT%third_party\imgui" /nologo /W4 /O2 /MT /EHsc %MHINC% /c"

%CXX% /Fo"%OUT%\stereo_proxy.obj" "%FC3ROOT%src\stereo_proxy.cpp" || exit /b 1
%CXX% /Fo"%OUT%\main_state.obj" "%FC3ROOT%src\main_state.cpp" || exit /b 1
%CXX% /Fo"%OUT%\stereo.obj" "%FC3ROOT%src\stereo.cpp" || exit /b 1
%CXX% /Fo"%OUT%\dxbc_patch.obj" "%FC3ROOT%src\dxbc_patch.cpp" || exit /b 1
%CXX% /Fo"%OUT%\dxbc_patch_pixel.obj" "%FC3ROOT%src\dxbc_patch_pixel.cpp" || exit /b 1
%CXX% /I"%FC3ROOT%third_party\openxr\include" /Fo"%OUT%\xr_presenter.obj" "%FC3ROOT%src\xr_presenter.cpp" || exit /b 1
%CXX% /Fo"%OUT%\fov_boost.obj" "%FC3ROOT%src\fov_boost.cpp" || exit /b 1
%CXX% /I"%FC3ROOT%third_party\vigemclient\include" /Fo"%OUT%\vr_pad.obj" "%FC3ROOT%src\vr_pad.cpp" || exit /b 1
%CXX% /I"%FC3ROOT%third_party\vigemclient\include" /I"%FC3ROOT%third_party\vigemclient\src" /W0 /Fo"%OUT%\vigem_client.obj" "%FC3ROOT%third_party\vigemclient\src\ViGEmClient.cpp" || exit /b 1
%CXX% /Fo"%OUT%\di_probe.obj" "%FC3ROOT%src\di_probe.cpp" || exit /b 1
%CXX% /Fo"%OUT%\aim_probe.obj" "%FC3ROOT%src\aim_probe.cpp" || exit /b 1
%CXX% /Fo"%OUT%\throwable_launch_probe.obj" "%FC3ROOT%src\throwable_launch_probe.cpp" || exit /b 1
%CXX% /Fo"%OUT%\throwable_capture_host.obj" "%FC3ROOT%src\throwable_capture_host.cpp" || exit /b 1
%CXX% /TC /Fo"%OUT%\mh_hook.obj" "%MH%\src\hook.c" || exit /b 1
%CXX% /TC /Fo"%OUT%\mh_buffer.obj" "%MH%\src\buffer.c" || exit /b 1
%CXX% /TC /Fo"%OUT%\mh_trampoline.obj" "%MH%\src\trampoline.c" || exit /b 1
%CXX% /TC /Fo"%OUT%\mh_hde32.obj" "%MH%\src\hde\hde32.c" || exit /b 1

%CXX% /Fo"%OUT%\imgui.obj" "%FC3ROOT%third_party\imgui\imgui.cpp" || exit /b 1
%CXX% /Fo"%OUT%\imgui_draw.obj" "%FC3ROOT%third_party\imgui\imgui_draw.cpp" || exit /b 1
%CXX% /Fo"%OUT%\imgui_widgets.obj" "%FC3ROOT%third_party\imgui\imgui_widgets.cpp" || exit /b 1
%CXX% /Fo"%OUT%\imgui_tables.obj" "%FC3ROOT%third_party\imgui\imgui_tables.cpp" || exit /b 1
%CXX% /Fo"%OUT%\imgui_impl_dx11.obj" "%FC3ROOT%third_party\imgui\backends\imgui_impl_dx11.cpp" || exit /b 1

link /nologo /DLL "%OUT%\stereo_proxy.obj" "%OUT%\main_state.obj" "%OUT%\stereo.obj" "%OUT%\dxbc_patch.obj" "%OUT%\dxbc_patch_pixel.obj" "%OUT%\xr_presenter.obj" "%OUT%\fov_boost.obj" "%OUT%\vr_pad.obj" "%OUT%\di_probe.obj" "%OUT%\aim_probe.obj" "%OUT%\throwable_launch_probe.obj" "%OUT%\throwable_capture_host.obj" "%OUT%\vigem_client.obj" "%OUT%\mh_hook.obj" "%OUT%\mh_buffer.obj" "%OUT%\mh_trampoline.obj" "%OUT%\mh_hde32.obj" "%OUT%\imgui.obj" "%OUT%\imgui_draw.obj" "%OUT%\imgui_widgets.obj" "%OUT%\imgui_tables.obj" "%OUT%\imgui_impl_dx11.obj" d3dcompiler.lib shell32.lib user32.lib advapi32.lib dxguid.lib setupapi.lib /DEF:"%FC3ROOT%src\d3d11.def" /OUT:"%OUT%\d3d11.dll" /IMPLIB:"%OUT%\d3d11.lib" || exit /b 1

cl /I"%FC3ROOT%third_party\imgui" /nologo /W4 /O2 /MT /EHsc /Fo"%OUT%\stereo_smoke.obj" "%FC3ROOT%tests\stereo_smoke.cpp" /link user32.lib d3dcompiler.lib /OUT:"%OUT%\stereo_smoke.exe" || exit /b 1
rem The offline test must never open a headset session (it renders in software).
set FC3_STEREO_NO_XR=1
"%OUT%\stereo_smoke.exe" "%OUT%\d3d11.dll"
exit /b %errorlevel%
