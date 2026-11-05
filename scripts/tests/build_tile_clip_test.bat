@echo off
set "FC3ROOT=%~dp0..\..\"
setlocal
call "%~dp0..\vsenv32.bat" || exit /b 1
set "OUT=%FC3ROOT%build\tests"
if not exist "%OUT%" mkdir "%OUT%"
cl /nologo /W3 /O2 /EHsc /Fo"%OUT%\\" "%FC3ROOT%tests\tile_clip_test.cpp" "%FC3ROOT%src\dxbc_patch_pixel.cpp" "%FC3ROOT%src\dxbc_patch.cpp" /link /OUT:"%OUT%\tile_clip_test.exe" || exit /b 1
"%OUT%\tile_clip_test.exe"
exit /b %errorlevel%
