@echo off
set "FC3ROOT=%~dp0..\..\"
call "%~dp0..\vsenv32.bat" || exit /b 1
cl /I"%FC3ROOT%third_party\imgui" /nologo /W4 /O2 /MT /EHsc /Fo"%FC3ROOT%build\stereo\fov_transition_test.obj" "%FC3ROOT%tests\fov_transition_test.cpp" /link shell32.lib /OUT:"%FC3ROOT%build\stereo\fov_transition_test.exe"
if errorlevel 1 exit /b 1
"%FC3ROOT%build\stereo\fov_transition_test.exe"
