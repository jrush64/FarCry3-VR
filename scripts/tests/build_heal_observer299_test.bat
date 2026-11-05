@echo off
set "FC3ROOT=%~dp0..\..\"
call "%~dp0..\vsenv32.bat" || exit /b 1
cl /I"%FC3ROOT%third_party\minhook\include" /nologo /O2 /MT /EHsc /Fo"%FC3ROOT%build\stereo\heal_observer299_test.obj" "%FC3ROOT%tests\heal_observer299_test.cpp" /link /OUT:"%FC3ROOT%build\stereo\heal_observer299_test.exe" || exit /b 1
"%FC3ROOT%build\stereo\heal_observer299_test.exe"
exit /b %errorlevel%
