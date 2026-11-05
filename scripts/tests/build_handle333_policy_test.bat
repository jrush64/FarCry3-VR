@echo off
set "FC3ROOT=%~dp0..\..\"
call "%~dp0..\vsenv32.bat" || exit /b 1
cl /nologo /O2 /MT /EHsc /Fo"%FC3ROOT%build\stereo\handle333_policy_test.obj" "%FC3ROOT%tests\handle333_policy_test.cpp" /link /OUT:"%FC3ROOT%build\stereo\handle333_policy_test.exe" || exit /b 1
"%FC3ROOT%build\stereo\handle333_policy_test.exe"
exit /b %errorlevel%
