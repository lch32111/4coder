I am studying `4ed` code base. This study would let me know how to write code editor and customize `4coder` more by myself. I will organize questions and answers for 4coder that I want to know.

[toc]



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