@echo off
set "FC3ROOT=%~dp0..\..\"
call "%~dp0..\vsenv32.bat" || exit /b 1
cl /nologo /O2 /MT /EHsc /I"%FC3ROOT%third_party\openxr\include" /Fo"%FC3ROOT%build\stereo\xr_body_pose_test.obj" "%FC3ROOT%tests\xr_body_pose_test.cpp" /link /OUT:"%FC3ROOT%build\stereo\xr_body_pose_test.exe"
if errorlevel 1 exit /b 1
"%FC3ROOT%build\stereo\xr_body_pose_test.exe"
