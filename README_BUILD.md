### How to Build

I currently change the build batch files for myself. So, It will not work as I think on another platform (Linux/Mac) (If I need to use this on another platform, then I will work on this). Anyway it will work in Windows platform.

Use "Developer Command Prompt for VS 2022" (or command prompt of other version) to use `cl.exe`. You can manually call `vcvarsall.bat` (I don't know the name is correct) on your command line terminal.

If you are ready to use `cl.exe`, then build like this:

```bat
cd 4coder/code
"bin/build.bat" release
```

After building it, you can see `4coder/build` folder and `4ed.exe`. you need to copy the `4coder/4coder-non-source/dist_files/fonts` folder into `4coder/build` folder in order to execute `4ed.exe`. 

Building the program is basically same as the way in the archive repository of 4coder. My change on "bin/build.bat" is preventing the change on files in `generated` folders. Because the files in `generated` folders change whenever you build, I had to cancel the changes by overwriting it in order to include the changes on my git repository. This same mechanism applies to the build batch of a custom code (4coder-fleury).

To apply the fleury's custom code, build like this:

```bat
cd 4coder/code/custom/4coder_fleury
build
```

This builds the custom library dll and them move the dll into `4coder/build` folder so that `4ed.exe` can use it. Whenever you build this custom code, I made the build batch overwrite `bindings.4coder` and `config.4coder` in the `4coder/build`. 

There you go! you can use 4coder now with the 4coder_fleury custom code.