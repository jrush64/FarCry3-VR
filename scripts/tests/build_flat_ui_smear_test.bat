@echo off
set "FC3ROOT=%~dp0..\..\"
call "%~dp0..\vsenv32.bat" || exit /b 1
cl /nologo /W4 /O2 /MT /EHsc /std:c++14 /Fo"%FC3ROOT%build\stereo\\" "%FC3ROOT%tests\flat_ui_smear_test.cpp" "%FC3ROOT%src\dxbc_patch.cpp" /link d3d11.lib d3dcompiler.lib /OUT:"%FC3ROOT%build\stereo\flat_ui_smear_test.exe"
if errorlevel 1 exit /b 1
"%FC3ROOT%build\stereo\flat_ui_smear_test.exe"
