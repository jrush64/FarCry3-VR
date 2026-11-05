@echo off
set "FC3ROOT=%~dp0..\..\"
setlocal
call "%~dp0..\vsenv32.bat" || exit /b 1
if errorlevel 1 exit /b 1
set "OUT=%FC3ROOT%build\stereo"
cl /I"%FC3ROOT%third_party\imgui" /I"%FC3ROOT%third_party\minhook\include" /nologo /W4 /O2 /MT /EHsc /c /Fo"%OUT%\stereo.obj" "%FC3ROOT%src\stereo.cpp" || exit /b 1
cl /I"%FC3ROOT%third_party\imgui" /I"%FC3ROOT%third_party\minhook\include" /I"%FC3ROOT%third_party\openxr\include" /nologo /W4 /O2 /MT /EHsc /c /Fo"%OUT%\xr_presenter.obj" "%FC3ROOT%src\xr_presenter.cpp" || exit /b 1
cl /I"%FC3ROOT%third_party\minhook\include" /I"%FC3ROOT%third_party\vigemclient\include" /nologo /W4 /O2 /MT /EHsc /c /Fo"%OUT%\vr_pad.obj" "%FC3ROOT%src\vr_pad.cpp" || exit /b 1
cl /I"%FC3ROOT%third_party\minhook\include" /nologo /W4 /O2 /MT /EHsc /c /Fo"%OUT%\aim_probe.obj" "%FC3ROOT%src\aim_probe.cpp" || exit /b 1
link /nologo /DLL "%OUT%\stereo_proxy.obj" "%OUT%\main_state.obj" "%OUT%\stereo.obj" "%OUT%\dxbc_patch.obj" "%OUT%\dxbc_patch_pixel.obj" "%OUT%\xr_presenter.obj" "%OUT%\fov_boost.obj" "%OUT%\vr_pad.obj" "%OUT%\di_probe.obj" "%OUT%\aim_probe.obj" "%OUT%\throwable_launch_probe.obj" "%OUT%\throwable_capture_host.obj" "%OUT%\vigem_client.obj" "%OUT%\mh_hook.obj" "%OUT%\mh_buffer.obj" "%OUT%\mh_trampoline.obj" "%OUT%\mh_hde32.obj" "%OUT%\imgui.obj" "%OUT%\imgui_draw.obj" "%OUT%\imgui_widgets.obj" "%OUT%\imgui_tables.obj" "%OUT%\imgui_impl_dx11.obj" d3dcompiler.lib shell32.lib user32.lib advapi32.lib dxguid.lib setupapi.lib /DEF:"%FC3ROOT%src\d3d11.def" /OUT:"%OUT%\d3d11.dll" /IMPLIB:"%OUT%\d3d11.lib" || exit /b 1

cl /I"%FC3ROOT%third_party\imgui" /nologo /W4 /O2 /MT /EHsc /Fo"%OUT%\stereo_smoke.obj" "%FC3ROOT%tests\stereo_smoke.cpp" /link user32.lib d3dcompiler.lib /OUT:"%OUT%\stereo_smoke.exe" || exit /b 1
rem The offline test must never open a headset session (it renders in software).
set FC3_STEREO_NO_XR=1
"%OUT%\stereo_smoke.exe" "%OUT%\d3d11.dll"
exit /b %errorlevel%
