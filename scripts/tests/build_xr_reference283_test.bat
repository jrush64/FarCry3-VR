@echo off
set "FC3ROOT=%~dp0..\..\"
call "%~dp0..\vsenv32.bat" || exit /b 1
set "OUT=%FC3ROOT%build\stereo"
cl /I"%FC3ROOT%third_party\openxr\include" /I"%FC3ROOT%third_party\imgui" /I"%FC3ROOT%third_party\minhook\include" /nologo /O2 /MT /EHsc /Fo"%OUT%\xr_reference283_test.obj" "%FC3ROOT%tests\xr_reference283_test.cpp" /link "%OUT%\stereo.obj" "%OUT%\di_probe.obj" "%OUT%\dxbc_patch_pixel.obj" "%OUT%\dxbc_patch.obj" "%OUT%\fov_boost.obj" "%OUT%\main_state.obj" "%OUT%\mh_buffer.obj" "%OUT%\mh_hde32.obj" "%OUT%\mh_hook.obj" "%OUT%\mh_trampoline.obj" "%OUT%\stereo_proxy.obj" "%OUT%\throwable_launch_probe.obj" "%OUT%\throwable_capture_host.obj" "%OUT%\vigem_client.obj" "%OUT%\vr_pad.obj" "%OUT%\aim_probe.obj" "%OUT%\imgui.obj" "%OUT%\imgui_draw.obj" "%OUT%\imgui_widgets.obj" "%OUT%\imgui_tables.obj" "%OUT%\imgui_impl_dx11.obj" d3dcompiler.lib d3d11.lib shell32.lib user32.lib advapi32.lib dxguid.lib setupapi.lib /OUT:"%OUT%\xr_reference283_test.exe"
if errorlevel 1 exit /b 1
"%OUT%\xr_reference283_test.exe"
exit /b %errorlevel%
