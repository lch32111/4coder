@echo off

REM NOTE(chan): argument for debug/release build
set mode=%1

REM NOTE(chan): copy the original files in generated, which will change after the build
REM I will overwrite the files with the original files after the build
copy ..\generated\command_metadata.h .\original_command_metadata.h
copy ..\generated\managed_id_metadata.cpp .\original_managed_id_metadata.cpp

call ..\bin\buildsuper_x64-win.bat .\4coder_fleury.cpp %mode%

move .\original_command_metadata.h ..\generated\command_metadata.h
move .\original_managed_id_metadata.cpp ..\generated\managed_id_metadata.cpp

REM NOTE(chan) : let's move the dll and pdb into the build folder
REM copy .\custom_4coder.dll ..\..\custom_4coder.dll
REM copy .\custom_4coder.pdb ..\..\custom_4coder.pdb
move .\custom_4coder.dll ..\..\..\build\custom_4coder.dll
move .\custom_4coder.pdb ..\..\..\build\custom_4coder.pdb

REM NOTE(chan) : delete unnecessary files
del .\vc140.pdb

REM NOTE(chan) : copy 4coder files for fleury customization
copy .\bindings.4coder ..\..\..\build
copy .\config.4coder ..\..\..\build