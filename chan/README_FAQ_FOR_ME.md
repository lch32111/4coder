I am studying `4ed` code base. This study would let me know how to write code editor and customize `4coder` more by myself. I will organize questions and answers for 4coder that I want to know.

[toc]



### Quick Shortcut

* Vsplit a panel : Ctrl + P
* Hsplit a panel : Ctrl + -
* Remove a panel : Ctrl + Shift + P
* Open a project: Ctrl + Shift + O



### How do you work on 4code codebase?

* Move the latest build executable into the latest-safe-build.
* Build with debug mode first with `chan_build.bat`. Or You can do by yourself with `README_BUILD.md`
* Open `project.4coder` with Ctrl + Shift + O in the 4coder/code.
* Change codes and build with x64 Developer Console.
* Debug with RADDEBUGER.



### How to change theme?

you can change `default_theme_name` in the `config.4coder`. As far as I know, 4ed looks for the `themes` folder in the directory of the `4ed.exe`. So, There should be `themes` folder in the directory of the `4ed.exe` and the theme `.4coder` file in the `themes` folder.



### How to call batch file within 4ed?

Use the command `build_in_build_panel`. This command looks for the `build.bat` file in the root folder(? not sure. I don't know exactly). 



### How to copy/paste for searching?

I asked this on 4coder discord channel. @longtran2904 answered me with the code to add this feature. You can do this by changing the 4coder code. I changed the code with his help and it works. The code I added on the `isearch` function of `code/custom/4coder_base_commands.cpp` is like this

```c++
else if (match_key_code(&in, KeyCode_V) && has_modifier(&in, KeyCode_Control))
{
    Scratch_Block scratch(app);
    String8 clipboard_string = push_clipboard_index(scratch, 0, 0);
    if (clipboard_string.size)
    {
        String_u8 bar_string = Su8(bar.string, sizeof(bar_string_space));
        string_append(&bar_string, clipboard_string);
        bar.string = bar_string.string;
        string_change = true;
    }
}
```

I think this is a good point to learn the 4coder codebase.



### How to find definitions in many files?

You may want to find the definitions of structures/classes. To search the definitions, you need to load the related files into the 4coder editor. You can do this easily for many files with the command `open_all_code`, which opens all files in the current directory. And then you can find the definitions by typing the definition name on the query bar with the command `f4_search_for_definition__project_wide`.



### I want to use an environment variable in "load_paths_custom".

I am using a `project.4coder` file to set up my project. One problem is that I am using the vulkan headers, which are located by the environment variable. Sometimes, I also want to look up the definition from the vulkan headers. In this case, I'd like to use the environment variable. So let's first find a code to read `load_paths_custom`.

When opening a project, we use `f4_open_project` (Ctrl+Shift+O). The code for this command is located on `4coder_fleury_base_commands.cpp`. I think the important function on it is `load_project` function. The `load_project` function is located at `4coder_project_commands.cpp`. I guess the `f4_open_project` is setting a current folder directory so that the `load_project` function can find a `project.4coder` file.

In `load_project`, I added the code that checks whether a path include `|` characters. If it includes them, then I get environment path and then replace it with the environment path.





