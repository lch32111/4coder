[toc]

### How to Build

I currently change the build batch files for myself. So, It will not work as I think on another platform (Linux/Mac) (If I need to use this on another platform, then I will work on this). Anyway it will work in Windows platform.

Use "x64 Native Tools Command Prompt for VS 2022" (or command prompt of other version) to use `cl.exe`. You can manually call `vcvarsall.bat` (I don't know the name is correct) on your command line terminal.

If you are ready to use `cl.exe`, then build like this:

```bat
cd 4coder/code
"bin/build.bat" release
```

After building it, you can see `4coder/build` folder and `4ed.exe`. you need to copy the `4coder/4coder-non-source/dist_files/fonts` folder into `4coder/build` folder in order to execute `4ed.exe`.  If you want to build for the debug mode, then just call `"bin/build.bat`.

Building the program is basically same as the way in the archive repository of 4coder. My change on "bin/build.bat" is preventing the change on files in `generated` folders. Because the files in `generated` folders change whenever you build, I had to cancel the changes by overwriting it in order to include the changes on my git repository. This same mechanism applies to the build batch of a custom code (4coder-fleury).

To apply the fleury's custom code, build like this:

```bat
cd 4coder/code/custom/4coder_fleury
build.bat release
```

This builds the custom library dll and them move the dll into `4coder/build` folder so that `4ed.exe` can use it. Whenever you build this custom code, I made the build batch overwrite `bindings.4coder` and `config.4coder` in the `4coder/build`. If you want to build for debug mode, you have to call `build.bat debug`.

There you go! you can use 4coder now with the 4coder_fleury custom code.



### Compile Errors

* If you get this error:

  ```
  4ed_build.cpp
  build.obj : error LNK2019: unresolved external symbol _GetCurrentDirectoryA referenced in function "void __cdecl build(struct Arena *,unsigned int,unsigned int,char *,char * *,char *,char *,char * *,char * *,char * *)" (?build@@YAXPAUArena@@IIPADPAPAD11222@Z)
  build.obj : error LNK2019: unresolved external symbol _SetCurrentDirectoryA referenced in function "void __cdecl build(struct Arena *,unsigned int,unsigned int,char *,char * *,char *,char *,char * *,char * *,char * *)" (?build@@YAXPAUArena@@IIPADPAPAD11222@Z)
  build.obj : error LNK2019: unresolved external symbol _QueryPerformanceFrequency referenced in function "struct Arena __cdecl fm_init_system(int)" (?fm_init_system@@YA?AUArena@@H@Z)
  build.obj : error LNK2019: unresolved external symbol _CreateDirectoryA referenced in function "void __cdecl fm_make_folder_if_missing(struct Arena *,char *)" (?fm_make_folder_if_missing@@YAXPAUArena@@PAD@Z)
  ```

  It means that you are using 32-bit version compiler. Check that you are using "x64 Native Tools Command Prompt for VS 2022".

  If you use 32-bit version, then you will also get these errors for compiler fleury custom codes:

  ```
  4coder_fleury.cpp
  D:\VisualStudioProject\4coder\code\custom\4coder_fleury\4coder_fleury_command_server.cpp(489): error C2220: the following warning is treated as an error
  D:\VisualStudioProject\4coder\code\custom\4coder_fleury\4coder_fleury_command_server.cpp(489): warning C4389: '==': signed/unsigned mismatch
  ```

  