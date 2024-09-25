@ECHO off

REM This batch file is located on "4coder/chan" folder.

SET "cur_call_working_dir=%cd%"
SET "slash_batch_dir=%~dp0%"
SET "cur_batch_dir=%slash_batch_dir:~0, -1%"
SET "coder_root_dir=%cur_batch_dir%\..\"
SET "code_root_dir=%coder_root_dir%\code"

SET "mode=%1"
if "%mode%" == "" (GOTO ERROR_BUILD_TARGET)

cd %code_root_dir%

REM build 4ed.exe
CALL bin\build.bat %mode%

cd %code_root_dir%\custom\4coder_fleury

REM build fleury "custom_4coder.dll"
CALL build.bat %mode%

cd %coder_root_dir%

REM copy fonts file
mkdir "build\fonts"
ROBOCOPY "4coder-non-source\dist_files\fonts" "build/fonts"

REM copy theme file
mkdir "build\themes"
ROBOCOPY "chan" "build\themes" "theme-chan.4coder"

REM copy config file
ROBOCOPY "chan" "build" "config.4coder"

goto EXIT

:ERROR_BUILD_TARGET
ECHO Type a build target (debug/release)

:EXIT
cd %cur_call_working_dir%
