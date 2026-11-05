@echo off
set "FC3ROOT=%~dp0..\..\"
setlocal
call "%~dp0..\vsenv32.bat" || exit /b 1
if errorlevel 1 exit /b 1
cl /nologo /W4 /O2 /MT /EHsc /I"%FC3ROOT%third_party\minhook\include" /Fo"%FC3ROOT%build\stereo\locker330_native_test.obj" "%FC3ROOT%tests\locker330_native_test.cpp" /link /OUT:"%FC3ROOT%build\stereo\locker330_native_test.exe"
if errorlevel 1 exit /b 1
"%FC3ROOT%build\stereo\locker330_native_test.exe"
exit /b %errorlevel%

