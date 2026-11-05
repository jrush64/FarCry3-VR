@echo off
set "FC3ROOT=%~dp0..\..\"
call "%~dp0..\vsenv32.bat" || exit /b 1
cl /I"%FC3ROOT%third_party\imgui" /nologo /W4 /O2 /MT /EHsc /I"%FC3ROOT%third_party\openxr\include" /Fo"%FC3ROOT%build\stereo\xr_hand_tracking_test.obj" "%FC3ROOT%tests\xr_hand_tracking_test.cpp" /link /OUT:"%FC3ROOT%build\stereo\xr_hand_tracking_test.exe"
if errorlevel 1 exit /b 1
"%FC3ROOT%build\stereo\xr_hand_tracking_test.exe"
