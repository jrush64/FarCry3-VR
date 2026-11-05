@echo off
set "FC3ROOT=%~dp0..\..\"
call "%~dp0..\vsenv32.bat" || exit /b 1
cl /nologo /O2 /MT /EHsc /c /Fo"%FC3ROOT%build\stereo\selector294_material_dxbc.obj" "%FC3ROOT%src\dxbc_patch.cpp" || exit /b 1
cl /nologo /O2 /MT /EHsc /Fo"%FC3ROOT%build\stereo\selector294_material_test.obj" "%FC3ROOT%tests\selector294_material_test.cpp" /link "%FC3ROOT%build\stereo\selector294_material_dxbc.obj" d3d11.lib d3dcompiler.lib dxguid.lib /OUT:"%FC3ROOT%build\stereo\selector294_material_test.exe" || exit /b 1
"%FC3ROOT%build\stereo\selector294_material_test.exe"
exit /b %errorlevel%
