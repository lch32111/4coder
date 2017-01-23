@echo off

call "ctime" -begin string\_4coder_string.ctm

SET OPTS=-W4 -wd4310 -wd4100 -wd4201 -wd4505 -wd4996 -wd4127 -wd4510 -wd4512 -wd4610 -wd4390 -WX
SET OPTS=%OPTS% -wd4611 -GR- -EHa- -nologo -FC

SET FirstError=0

pushd ..\build
cl %OPTS% ..\code\string\string_builder.cpp /Zi /Festring_builder
if %ERRORLEVEL% neq 0 (set FirstError=1)
if %ERRORLEVEL% neq 0 (goto END)
popd

pushd string
..\..\build\string_builder
if %ERRORLEVEL% neq 0 (set FirstError=1)
popd

:END
call "ctime" -end string\_4coder_string.ctm %FirstError%