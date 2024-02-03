@echo off

set location=%cd%
set me="%~dp0"
cd %me%
cd ..
set src_root=%cd%
cd ..\build
set build_root=%cd%
set bin_root=%src_root%\bin
set custom_root=%src_root%\custom
set custom_bin=%custom_root\bin
cd %location%

set mode=%1
if "%mode%" == "" (set mode="/DDEV_BUILD")

set opts=/W4 /wd4310 /wd4100 /wd4201 /wd4505 /wd4996 /wd4127 /wd4510 /wd4512 /wd4610 /wd4390 /wd4189 /WX
set opts=%opts% /GR- /EHa- /nologo /FC /Zi
set opts=%opts% /I%src_root% /I%custom_root%
set opts=%opts% %mode%

REM NOTE(chan) : change the output of the cl
set opts=%opts% /Fo%build_root%

set FirstError=0
pushd %build_root%
call cl %opts% kernel32.lib %bin_root%\4ed_build.cpp /Febuild
if %ERRORLEVEL% neq 0 (set FirstError=1)
if %ERRORLEVEL% neq 0 (goto END)
popd

REM NOTE(chan) : I don't know why build.obj is not created on build folder
REM so I move manually here
move %src_root%\..\build.obj %build_root%

REM NOTE(chan) : code\custom\generated\command_metadata.h is changed in compiling.
REM so I copy the original one temporarily and paste it to the changed one after building the program.
REM this helps to remove boring jobs for git code changes
copy %src_root%\custom\generated\command_metadata.h %src_root%\..\original_command_metadata.h

%build_root%\build

REM The building is done. Let's overwrite the changed metadata.h with the original one
move %src_root%\..\original_command_metadata.h %src_root%\custom\generated\command_metadata.h

:END
if %ERRORLEVEL% neq 0 (set FirstError=1)
