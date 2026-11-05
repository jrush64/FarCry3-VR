@echo off
set "FC3ROOT=%~dp0..\..\"
call "%~dp0..\vsenv32.bat" || exit /b 1
set "OUT=%FC3ROOT%build\stereo"
cl /I"%FC3ROOT%third_party\imgui" /nologo /O2 /MT /EHsc /Fo"%OUT%\menu_ray298_bridge_test.obj" "%FC3ROOT%tests\menu_ray298_bridge_test.cpp" /link "%OUT%\imgui.obj" "%OUT%\imgui_draw.obj" "%OUT%\imgui_widgets.obj" "%OUT%\imgui_tables.obj" "%OUT%\imgui_impl_dx11.obj" d3d11.lib d3dcompiler.lib user32.lib /OUT:"%OUT%\menu_ray298_bridge_test.exe"
if errorlevel 1 exit /b 1
"%OUT%\menu_ray298_bridge_test.exe"
exit /b %errorlevel%
