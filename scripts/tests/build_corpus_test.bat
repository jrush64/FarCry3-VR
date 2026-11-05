@echo off
set "FC3ROOT=%~dp0..\..\"
setlocal
call "%~dp0..\vsenv32.bat" || exit /b 1
if errorlevel 1 exit /b 1
if not exist "%FC3ROOT%build" mkdir "%FC3ROOT%build"
cl /I"%FC3ROOT%third_party\imgui" /nologo /W4 /O2 /MT /EHsc /Fo"%FC3ROOT%build\\" "%FC3ROOT%tests\patch_corpus.cpp" "%FC3ROOT%src\dxbc_patch.cpp" "%FC3ROOT%src\dxbc_patch_pixel.cpp" /link d3d11.lib d3dcompiler.lib /OUT:"%FC3ROOT%build\corpus_check.exe"
exit /b %errorlevel%
