@echo off
set "FC3ROOT=%~dp0..\..\"
setlocal
call "%~dp0..\vsenv32.bat" || exit /b 1
if errorlevel 1 exit /b 1
if not exist "%FC3ROOT%build\throwables143" mkdir "%FC3ROOT%build\throwables143"
cl /nologo /W4 /O2 /MT /EHsc /Fo"%FC3ROOT%build\throwables143\policy.obj" "%FC3ROOT%tests\manual_throwables143_test.cpp" /Fe:"%FC3ROOT%build\throwables143\policy.exe"
if errorlevel 1 exit /b 1
"%FC3ROOT%build\throwables143\policy.exe"
if errorlevel 1 exit /b 1
cl /nologo /W4 /O2 /MT /EHsc /I"%FC3ROOT%third_party\minhook\include" /Fo"%FC3ROOT%build\throwables143\launch.obj" "%FC3ROOT%tests\throwable_launch143_test.cpp" "%FC3ROOT%build\stereo\mh_buffer.obj" "%FC3ROOT%build\stereo\mh_hde32.obj" "%FC3ROOT%build\stereo\mh_hook.obj" "%FC3ROOT%build\stereo\mh_trampoline.obj" /Fe:"%FC3ROOT%build\throwables143\launch.exe"
if errorlevel 1 exit /b 1
pushd "%FC3ROOT%build\throwables143"
launch.exe
set "THROWABLE_RESULT=%errorlevel%"
popd
exit /b %THROWABLE_RESULT%
