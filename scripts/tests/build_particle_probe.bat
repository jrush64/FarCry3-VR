@echo off
set "FC3ROOT=%~dp0..\..\"
call "%~dp0..\vsenv32.bat" || exit /b 1
if not exist "%FC3ROOT%build\tests" mkdir "%FC3ROOT%build\tests"
cl /nologo /O2 /EHsc /Fo"%FC3ROOT%build\tests\\" "%FC3ROOT%tests\tile_clip_test.cpp" "%FC3ROOT%src\dxbc_patch_pixel.cpp" "%FC3ROOT%src\dxbc_patch.cpp" /link /OUT:"%FC3ROOT%build\tests\particle_before.exe"
if errorlevel 1 exit /b 1
cl /nologo /O2 /EHsc /Fo"%FC3ROOT%build\tests\\" "%FC3ROOT%tests\tile_clip_test.cpp" "%FC3ROOT%src\dxbc_patch_pixel.cpp" "%FC3ROOT%src\dxbc_patch_particle_experiment.cpp" /link /OUT:"%FC3ROOT%build\tests\particle_after.exe"
exit /b %errorlevel%
