@echo off
set "FC3ROOT=%~dp0..\..\"
call "%~dp0..\vsenv32.bat" || exit /b 1
if errorlevel 1 exit /b 1
cl /nologo /W4 /O2 /MT /EHsc /c /Fo"%FC3ROOT%build\stereo\locker329_probe_bridge.obj" "%FC3ROOT%tests\locker329_probe_bridge.cpp"
if errorlevel 1 exit /b 1
cl /nologo /W4 /O2 /MT /EHsc /Fo"%FC3ROOT%build\stereo\locker329_probe_test.obj" "%FC3ROOT%tests\locker329_probe_test.cpp" /link "%FC3ROOT%build\stereo\locker329_probe_bridge.obj" /OUT:"%FC3ROOT%build\stereo\locker329_probe_test.exe"
if errorlevel 1 exit /b 1
"%FC3ROOT%build\stereo\locker329_probe_test.exe"
exit /b %errorlevel%
