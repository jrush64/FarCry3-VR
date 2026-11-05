@echo off
set "FC3ROOT=%~dp0..\..\"
call "%~dp0..\vsenv32.bat" || exit /b 1
cl /nologo /W4 /O2 /MT /EHsc /c /I"%FC3ROOT%third_party\openxr\include" /Fo"%FC3ROOT%build\stereo\xr_presenter.obj" "%FC3ROOT%src\xr_presenter.cpp"
