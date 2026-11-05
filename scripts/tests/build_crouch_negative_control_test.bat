@echo off
set "FC3ROOT=%~dp0..\..\"
call "%~dp0..\vsenv32.bat" || exit /b 1
cl /I"%FC3ROOT%third_party\imgui" /nologo /W4 /O2 /MT /EHsc /I"%FC3ROOT%third_party\minhook\include" /I"%FC3ROOT%third_party\vigemclient\include" /Fo"%FC3ROOT%build\stereo\crouch_negative_control_test.obj" "%FC3ROOT%tests\crouch_negative_control_test.cpp" /link "%FC3ROOT%build\stereo\vigem_client.obj" setupapi.lib user32.lib /OUT:"%FC3ROOT%build\stereo\crouch_negative_control_test.exe"
if errorlevel 1 exit /b 1
"%FC3ROOT%build\stereo\crouch_negative_control_test.exe"
