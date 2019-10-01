/*
4coder_metadata_generator.cpp - A preprocessor program for generating a list of commands and their descriptions.
*/

// TOP

#define COMMAND_METADATA_OUT "generated/command_metadata.h"

#include "4coder_base_types.h"
#include "4coder_token.h"
#include "languages/generated_lexer_cpp.h"

#include "4coder_base_types.cpp"
#include "4coder_stringf.cpp"
#include "4coder_malloc_allocator.cpp"

#include "4coder_token.cpp"
#include "languages/generated_lexer_cpp.cpp"

#include "4coder_file.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#define str_to_l_c(s) ((i32)(s).size), ((s).str)
#define str_to_c_l(s) ((s).str), ((i32)(s).size)

///////////////////////////////

struct Line_Column_Coordinates{
    i64 line;
    i64 column;
};

static Line_Column_Coordinates
line_column_coordinates(String_Const_u8 text, i64 pos){
    if (pos < 0){
        pos = 0;
    }
    if (pos > (i64)text.size){
        pos = (i64)text.size;
    }
    
    Line_Column_Coordinates coords = {};
    coords.line = 1;
    coords.column = 1;
    u8 *end = text.str + pos;
    for (u8 *p = text.str; p < end; ++p){
        if (*p == '\n'){
            ++coords.line;
            coords.column = 1;
        }
        else{
            ++coords.column;
        }
    }
    
    return(coords);
}

static i64
line_number(String_Const_u8 text, i64 pos){
    Line_Column_Coordinates coords = line_column_coordinates(text, pos);
    return(coords.line);
}

static void
error(u8 *source_name, String_Const_u8 text, i64 pos, u8 *msg){
    Line_Column_Coordinates coords = line_column_coordinates(text, pos);
    fprintf(stdout, "%s:%lld:%lld: %s\n",
            source_name, coords.line, coords.column, msg);
    fflush(stdout);
}

///////////////////////////////

struct Reader{
    Arena *error_arena;
    u8 *source_name;
    String_Const_u8 text;
    Token_Array tokens;
    Token *ptr;
};

struct Temp_Read{
    Reader *reader;
    Token *pos;
};

static Reader
make_reader(Token_Array array, u8 *source_name, String_Const_u8 text){
    Reader reader = {};
    reader.tokens = array;
    reader.ptr = array.tokens;
    reader.source_name = source_name;
    reader.text = text;
    return(reader);
}

static Token
prev_token(Reader *reader){
    Token result = {};
    
    for (;;){
        if (reader->ptr > reader->tokens.tokens + reader->tokens.count){
            reader->ptr = reader->tokens.tokens + reader->tokens.count;
        }
        
        if (reader->ptr > reader->tokens.tokens){
            --reader->ptr;
            result = *reader->ptr;
        }
        else{
            reader->ptr = reader->tokens.tokens;
            memset(&result, 0, sizeof(result));
            break;
        }
        
        if (result.kind != TokenBaseKind_Comment &&
            result.kind != TokenBaseKind_Whitespace &&
            result.kind != TokenBaseKind_LexError){
            break;
        }
    }
    
    return(result);
}

static Token
get_token(Reader *reader){
    Token result = {};
    
    for (;;){
        if (reader->ptr < reader->tokens.tokens){
            reader->ptr = reader->tokens.tokens;
        }
        
        if (reader->ptr < reader->tokens.tokens + reader->tokens.count){
            result = *reader->ptr;
            ++reader->ptr;
        }
        else{
            reader->ptr = reader->tokens.tokens + reader->tokens.count;
            memset(&result, 0, sizeof(result));
            result.pos = reader->text.size;
            break;
        }
        
        if (result.kind != TokenBaseKind_Comment &&
            result.kind != TokenBaseKind_Whitespace &&
            result.kind != TokenBaseKind_LexError){
            break;
        }
    }
    
    return(result);
}

static Token
peek_token(Reader *reader){
    Token result = {};
    
    if (reader->ptr < reader->tokens.tokens){
        reader->ptr = reader->tokens.tokens;
    }
    
    if (reader->ptr >= reader->tokens.tokens + reader->tokens.count){
        result.pos = reader->text.size;
    }
    else{
        result = *reader->ptr;
    }
    
    return(result);
}

static i64
peek_pos(Reader *reader){
    Token token = peek_token(reader);
    return(token.pos);
}

static i64
line_number(Reader *reader, i64 pos){
    return(line_number(reader->text, pos));
}

static void
error(Reader *reader, i64 pos, u8 *msg){
    error(reader->source_name, reader->text, pos, msg);
}

static Temp_Read
begin_temp_read(Reader *reader){
    Temp_Read temp = {};
    temp.reader = reader;
    temp.pos = reader->ptr;
    return(temp);
}

static void
end_temp_read(Temp_Read temp){
    temp.reader->ptr = temp.pos;
}

///////////////////////////////

static String_Const_u8
token_str(String_Const_u8  text, Token token){
    String_Const_u8 str = string_prefix(string_skip(text, token.pos), token.size);
    return(str);
}

///////////////////////////////

typedef u32 Meta_Command_Entry_Type;
enum{
    MetaCommandEntry_DocString,
    MetaCommandEntry_Alias,
};

struct Meta_Command_Entry{
    Meta_Command_Entry *next;
    String_Const_u8 name;
    u8 *source_name;
    i64 line_number;
    union{
        struct{
            String_Const_u8 doc;
        } docstring;
        struct{
            String_Const_u8 potential;
        } alias;
    };
};

struct Meta_Command_Entry_Arrays{
    Meta_Command_Entry *first_doc_string;
    Meta_Command_Entry *last_doc_string;
    i32 doc_string_count;
    
    Meta_Command_Entry *first_alias;
    Meta_Command_Entry *last_alias;
    i32 alias_count;
};

///////////////////////////////

static i32
quick_sort_part(Meta_Command_Entry **entries, i32 first, i32 one_past_last){
    i32 pivot = one_past_last - 1;
    String_Const_u8 pivot_key = entries[pivot]->name;
    i32 j = first;
    for (i32 i = first; i < pivot; ++i){
        if (string_compare(entries[i]->name, pivot_key) < 0){
            Swap(Meta_Command_Entry*, entries[i], entries[j]);
            ++j;
        }
    }
    Swap(Meta_Command_Entry*, entries[j], entries[pivot]);
    return(j);
}

static void
quick_sort(Meta_Command_Entry **entries, i32 first, i32 one_past_last){
    if (first + 1 < one_past_last){
        i32 pivot = quick_sort_part(entries, first, one_past_last);
        quick_sort(entries, first, pivot);
        quick_sort(entries, pivot + 1, one_past_last);
    }
}

static Meta_Command_Entry**
get_sorted_meta_commands(Arena *arena, Meta_Command_Entry *first, i32 count){
    Meta_Command_Entry **entries = push_array(arena, Meta_Command_Entry*, count);
    
    i32 i = 0;
    for (Meta_Command_Entry *entry = first;
         entry != 0;
         entry = entry->next, ++i){
        entries[i] = entry;
    }
    Assert(i == count);
    
    quick_sort(entries, 0, count);
    
    return(entries);
}

///////////////////////////////

static b32
has_duplicate_entry(Meta_Command_Entry *first, String_Const_u8 name){
    b32 has_duplicate = false;
    for (Meta_Command_Entry *entry = first;
         entry != 0;
         entry = entry->next){
        if (string_match(name, entry->name)){
            has_duplicate = true;
        }
    }
    return(has_duplicate);
}

///////////////////////////////

static b32
require_key_identifier(Reader *reader, String_Const_u8 string, i64 *opt_pos_out){
    b32 success = false;
    
    Token token = get_token(reader);
    if (token.kind == TokenBaseKind_Identifier){
        String_Const_u8 lexeme = token_str(reader->text, token);
        if (string_match(lexeme, string)){
            success = true;
            if (opt_pos_out != 0){
                *opt_pos_out = token.pos;
            }
        }
    }
    
    if (!success){
        Temp_Memory temp = begin_temp(reader->error_arena);
        String_Const_u8 error_string = push_u8_stringf(reader->error_arena, "expected to find '%.*s'",
                                                       string.size, string.str);
        error(reader, token.pos, error_string.str);
        end_temp(temp);
    }
    
    return(success);
}

static b32
require_key_identifier(Reader *reader, char *str, i64 *opt_pos_out){
    return(require_key_identifier(reader, SCu8(str), opt_pos_out));
}

static b32
require_key_identifier(Reader *reader, String_Const_u8 string){
    return(require_key_identifier(reader, string, 0));
}

static b32
require_key_identifier(Reader *reader, char *str){
    return(require_key_identifier(reader, SCu8(str), 0));
}

static b32
require_open_parenthese(Reader *reader, i64 *opt_pos_out){
    b32 success = false;
    Token token = get_token(reader);
    if (token.kind == TokenBaseKind_ParentheticalOpen){
        success = true;
        if (opt_pos_out != 0){
            *opt_pos_out = token.pos;
        }
    }
    if (!success){
        error(reader, token.pos, (u8*)"expected to find '('");
    }
    return(success);
}

static b32
require_open_parenthese(Reader *reader){
    return(require_open_parenthese(reader, 0));
}

static b32
require_close_parenthese(Reader *reader, i64 *opt_pos_out){
    b32 success = false;
    Token token = get_token(reader);
    if (token.kind == TokenBaseKind_ParentheticalClose){
        success = true;
        if (opt_pos_out != 0){
            *opt_pos_out = token.pos;
        }
    }
    if (!success){
        error(reader, token.pos, (u8*)"expected to find ')'");
    }
    return(success);
}

static b32
require_close_parenthese(Reader *reader){
    return(require_close_parenthese(reader, 0));
}

static b32
require_comma(Reader *reader, i64 *opt_pos_out){
    b32 success = false;
    
    Token token = get_token(reader);
    if (token.sub_kind == TokenCppKind_Comma){
        success = true;
        if (opt_pos_out != 0){
            *opt_pos_out = token.pos;
        }
    }
    
    if (!success){
        error(reader, token.pos, (u8*)"expected to find ','");
    }
    
    return(success);
}

static b32
require_comma(Reader *reader){
    return(require_comma(reader, 0));
}

static b32
require_define(Reader *reader, i64 *opt_pos_out){
    b32 success = false;
    
    Token token = get_token(reader);
    if (token.sub_kind == TokenCppKind_PPDefine){
        success = true;
        if (opt_pos_out != 0){
            *opt_pos_out = token.pos;
        }
    }
    
    if (!success){
        error(reader, token.pos, (u8*)"expected to find '#define'");
    }
    
    return(success);
}

static b32
require_define(Reader *reader){
    return(require_define(reader, 0));
}

static b32
extract_identifier(Reader *reader, String_Const_u8 *str_out, i64 *opt_pos_out){
    b32 success = false;
    
    Token token = get_token(reader);
    if (token.kind == TokenBaseKind_Identifier){
        String_Const_u8 lexeme = token_str(reader->text, token);
        *str_out = lexeme;
        success = true;
        if (opt_pos_out != 0){
            *opt_pos_out = token.pos;
        }
    }
    
    if (!success){
        error(reader, token.pos, (u8*)"expected to find an identifier");
    }
    
    return(success);
}

static b32
extract_identifier(Reader *reader, String_Const_u8 *str_out){
    return(extract_identifier(reader, str_out, 0));
}

static b32
extract_integer(Reader *reader, String_Const_u8 *str_out, i64 *opt_pos_out){
    b32 success = false;
    
    Token token = get_token(reader);
    if (token.kind == TokenBaseKind_LiteralInteger){
        String_Const_u8 lexeme = token_str(reader->text, token);
        *str_out = lexeme;
        success = true;
        if (opt_pos_out != 0){
            *opt_pos_out = token.pos;
        }
    }
    
    if (!success){
        error(reader, token.pos, (u8*)"expected to find an integer");
    }
    
    return(success);
}


static b32
extract_integer(Reader *reader, String_Const_u8 *str_out){
    return(extract_integer(reader, str_out, 0));
}

static b32
extract_string(Reader *reader, String_Const_u8 *str_out, i64 *opt_pos_out){
    b32 success = false;
    
    Token token = get_token(reader);
    if (token.kind == TokenBaseKind_LiteralString){
        String_Const_u8 lexeme = token_str(reader->text, token);
        *str_out = lexeme;
        success = true;
        if (opt_pos_out != 0){
            *opt_pos_out = token.pos;
        }
    }
    
    if (!success){
        error(reader, token.pos, (u8*)"expected to find a string literal");
    }
    
    return(success);
}

static b32
extract_string(Reader *reader, String_Const_u8 *str_out){
    return(extract_string(reader, str_out, 0));
}

static b32
parse_documented_command(Arena *arena, Meta_Command_Entry_Arrays *arrays, Reader *reader){
    String_Const_u8 name = {};
    String_Const_u8 file_name = {};
    String_Const_u8 line_number = {};
    String_Const_u8 doc = {};
    
    // Getting the command's name
    i64 start_pos = 0;
    if (!require_key_identifier(reader, "CUSTOM_COMMAND_SIG", &start_pos)){
        return(false);
    }
    
    if (!require_open_parenthese(reader)){
        return(false);
    }
    
    if (!extract_identifier(reader, &name)){
        return(false);
    }
    
    if (!require_comma(reader)){
        return(false);
    }
    
    if (!extract_string(reader, &file_name)){
        return(false);
    }
    
    if (!require_comma(reader)){
        return(false);
    }
    
    if (!extract_integer(reader, &line_number)){
        return(false);
    }
    
    if (!require_close_parenthese(reader)){
        return(false);
    }
    
    // Getting the command's doc string
    if (!require_key_identifier(reader, "CUSTOM_DOC")){
        return(false);
    }
    
    if (!require_open_parenthese(reader)){
        return(false);
    }
    
    i64 doc_pos = 0;
    if (!extract_string(reader, &doc, &doc_pos)){
        return(false);
    }
    
    if (doc.size < 1 || doc.str[0] != '"'){
        error(reader, doc_pos, (u8*)"warning: doc strings with string literal prefixes not allowed");
        return(false);
    }
    
    if (!require_close_parenthese(reader)){
        return(false);
    }
    
    if (has_duplicate_entry(arrays->first_doc_string, name)){
        error(reader, start_pos, (u8*)"warning: multiple commands with the same name and separate doc strings, skipping this one");
        return(false);
    }
    
    doc = string_chop(string_skip(doc, 1), 1);
    
    String_Const_u8 file_name_unquoted = string_chop(string_skip(file_name, 1), 1);
    String_Const_u8 source_name = string_interpret_escapes(arena, file_name_unquoted);
    
    Meta_Command_Entry *new_entry = push_array(arena, Meta_Command_Entry, 1);
    new_entry->name = name;
    new_entry->source_name = source_name.str;
    new_entry->line_number = (i32)string_to_integer(line_number, 10);
    new_entry->docstring.doc = doc;
    sll_queue_push(arrays->first_doc_string, arrays->last_doc_string, new_entry);
    ++arrays->doc_string_count;
    
    return(true);
}

static b32
parse_alias(Arena *arena, Meta_Command_Entry_Arrays *arrays, Reader *reader){
    String_Const_u8 name = {};
    String_Const_u8 potential = {};
    
    // Getting the alias's name
    i64 start_pos = 0;
    if (!require_define(reader, &start_pos)){
        return(false);
    }
    
    if (!extract_identifier(reader, &name)){
        return(false);
    }
    
    // Getting the alias's target
    if (!require_key_identifier(reader, "CUSTOM_ALIAS")){
        return(false);
    }
    
    if (!require_open_parenthese(reader)){
        return(false);
    }
    
    if (!extract_identifier(reader, &potential)){
        return(false);
    }
    
    if (!require_close_parenthese(reader)){
        return(false);
    }
    
    Meta_Command_Entry *new_entry = push_array(arena, Meta_Command_Entry, 1);
    new_entry->name = name;
    new_entry->source_name = reader->source_name;
    new_entry->line_number = line_number(reader, start_pos);
    new_entry->alias.potential = potential;
    sll_queue_push(arrays->first_alias, arrays->last_alias, new_entry);
    ++arrays->alias_count;
    
    return(true);
}

///////////////////////////////

static void
parse_text(Arena *arena, Meta_Command_Entry_Arrays *entry_arrays, u8 *source_name, String_Const_u8 text){
    Token_List token_list = lex_full_input_cpp(arena, text);
    Token_Array array = token_array_from_list(arena, &token_list);
    
    Reader reader_ = make_reader(array, source_name, text);
    Reader *reader = &reader_;
    
    for (;;){
        Token token = get_token(reader);
        
        if (token.kind == TokenBaseKind_Identifier){
            String_Const_u8 lexeme = token_str(text, token);
            
            b32 in_preproc_body = HasFlag(token.flags, TokenBaseFlag_PreprocessorBody);
            
            if (!in_preproc_body && string_match(lexeme, string_u8_litexpr("CUSTOM_DOC"))){
                Temp_Read temp_read = begin_temp_read(reader);
                
                b32 found_start_pos = false;
                for (i32 R = 0; R < 10; ++R){
                    Token p_token = prev_token(reader);
                    if (p_token.kind == TokenBaseKind_Identifier){
                        String_Const_u8 p_lexeme = token_str(text, p_token);
                        if (string_match(p_lexeme, string_u8_litexpr("CUSTOM_COMMAND_SIG"))){
                            found_start_pos = true;
                            break;
                        }
                    }
                    if (p_token.kind == TokenBaseKind_EOF){
                        break;
                    }
                }
                
                if (!found_start_pos){
                    end_temp_read(temp_read);
                }
                else{
                    if (!parse_documented_command(arena, entry_arrays, reader)){
                        end_temp_read(temp_read);
                    }
                }
            }
            else if (string_match(lexeme, string_u8_litexpr("CUSTOM_ALIAS"))){
                Temp_Read temp_read = begin_temp_read(reader);
                
                b32 found_start_pos = false;
                for (i32 R = 0; R < 3; ++R){
                    Token p_token = prev_token(reader);
                    if (p_token.sub_kind == TokenCppKind_PPDefine){
                        if (R == 2){
                            found_start_pos = true;
                        }
                        break;
                    }
                    if (p_token.kind == TokenBaseKind_EOF){
                        break;
                    }
                }
                
                if (!found_start_pos){
                    end_temp_read(temp_read);
                }
                else{
                    if (!parse_alias(arena, entry_arrays, reader)){
                        end_temp_read(temp_read);
                    }
                }
            }
        }
        
        if (token.kind == TokenBaseKind_EOF){
            break;
        }
    }
}

static void
parse_file(Arena *arena, Meta_Command_Entry_Arrays *entry_arrays, Filename_Character *name_, i32 len){
    char *name = unencode(arena, name_, len);
    if (name == 0){
        if (sizeof(*name_) == 2){
            fprintf(stdout, "warning: could not unencode file name %ls - file skipped\n", (wchar_t*)name_);
        }
        else{
            fprintf(stdout, "warning: could not unencode file name %s - file skipped\n", (char*)name_);
        }
        return;
    }
    
    String_Const_u8 text = file_dump(arena, name);
    parse_text(arena, entry_arrays, (u8*)name, text);
}

static void
parse_files_by_pattern(Arena *arena, Meta_Command_Entry_Arrays *entry_arrays, Filename_Character *pattern, b32 recursive){
    Cross_Platform_File_List list = get_file_list(arena, pattern, filter_all);
    for (i32 i = 0; i < list.count; ++i){
        Cross_Platform_File_Info *info = &list.info[i];
        
        String_Const_Any info_name = SCany(info->name, info->len);
        Temp_Memory temp = begin_temp(arena);
        String_Const_u8 info_name_ascii = string_u8_from_any(arena, info_name);
        b32 is_generated = string_match(info_name_ascii, string_u8_litexpr("4coder_generated"));
        end_temp(temp);
        
        if (info->is_folder && is_generated){
            continue;
        }
        if (!recursive && info->is_folder){
            continue;
        }
        
        i32 full_name_len = list.path_length + 1 + info->len;
        if (info->is_folder){
            full_name_len += 2;
        }
        Filename_Character *full_name = push_array(arena, Filename_Character, full_name_len + 1);
        
        if (full_name == 0){
            fprintf(stdout, "fatal error: not enough memory to recurse to sub directory\n");
            exit(1);
        }
        
        memmove(full_name, list.path_name, list.path_length*sizeof(*full_name));
        full_name[list.path_length] = SLASH;
        memmove(full_name + list.path_length + 1, info->name, info->len*sizeof(*full_name));
        full_name[full_name_len] = 0;
        
        if (!info->is_folder){
            parse_file(arena, entry_arrays, full_name, full_name_len);
        }
        else{
            full_name[full_name_len - 2] = SLASH;
            full_name[full_name_len - 1] = '*';
            parse_files_by_pattern(arena, entry_arrays, full_name, true);
        }
    }
}

static void
show_usage(int argc, char **argv){
    char *name = "metadata_generator";
    if (argc >= 1){
        name = argv[0];
    }
    fprintf(stdout, "usage:\n%s [-R] <4coder-root-directory> <input-file-pattern> [<input-file-pattern> ...]\n", name);
    exit(0);
}

int
main(int argc, char **argv){
    if (argc < 3){
        show_usage(argc, argv);
    }
    
    b32 recursive = string_match(SCu8(argv[1]), string_u8_litexpr("-R"));
    if (recursive && argc < 4){
        show_usage(argc, argv);
    }
    
    Arena arena_ = make_arena_malloc(MB(1), 8);
    Arena *arena = &arena_;
    
    char *out_directory = argv[2];
    
    i32 start_i = 2;
    if (recursive){
        start_i = 3;
    }
    
    printf("metadata_generator ");
    for (i32 i = start_i; i < argc; i += 1){
        printf("%s ", argv[i]);
    }
    printf("\n");
    fflush(stdout);
    
    Meta_Command_Entry_Arrays entry_arrays = {};
    for (i32 i = start_i; i < argc; ++i){
        Filename_Character *pattern_name = encode(arena, argv[i]);
        parse_files_by_pattern(arena, &entry_arrays, pattern_name, recursive);
    }
    
    umem out_dir_len = cstring_length(out_directory);
    if (out_directory[0] == '"'){
        out_directory += 1;
        out_dir_len -= 2;
    }
    
    {
        String_Const_u8 str = SCu8(out_directory, out_dir_len);
        str = string_skip_chop_whitespace(str);
        out_directory = (char*)str.str;
        out_dir_len = str.size;
    }
    
    umem len = out_dir_len + 1 + sizeof(COMMAND_METADATA_OUT) - 1;
    char *out_file_name = (char*)malloc(len + 1);
    memcpy(out_file_name, out_directory, out_dir_len);
    memcpy(out_file_name + out_dir_len, "/", 1);
    memcpy(out_file_name + out_dir_len + 1, COMMAND_METADATA_OUT, sizeof(COMMAND_METADATA_OUT));
    
    FILE *out = fopen(out_file_name, "wb");
    
    if (out != 0){
        i32 entry_count = entry_arrays.doc_string_count;
        Meta_Command_Entry **entries = get_sorted_meta_commands(arena, entry_arrays.first_doc_string, entry_count);
        
        fprintf(out, "#if !defined(META_PASS)\n");
        fprintf(out, "#define command_id(c) (fcoder_metacmd_ID_##c)\n");
        fprintf(out, "#define command_metadata(c) (&fcoder_metacmd_table[command_id(c)])\n");
        fprintf(out, "#define command_metadata_by_id(id) (&fcoder_metacmd_table[id])\n");
        fprintf(out, "#define command_one_past_last_id %d\n", entry_arrays.doc_string_count);
        fprintf(out, "#if defined(CUSTOM_COMMAND_SIG)\n");
        fprintf(out, "#define PROC_LINKS(x,y) x\n");
        fprintf(out, "#else\n");
        fprintf(out, "#define PROC_LINKS(x,y) y\n");
        fprintf(out, "#endif\n");
        
        fprintf(out, "#if defined(CUSTOM_COMMAND_SIG)\n");
        for (i32 i = 0; i < entry_count; ++i){
            Meta_Command_Entry *entry = entries[i];
            fprintf(out, "CUSTOM_COMMAND_SIG(%.*s);\n", str_to_l_c(entry->name));
        }
        fprintf(out, "#endif\n");
        
        fprintf(out,
                "struct Command_Metadata{\n"
                "PROC_LINKS(Custom_Command_Function, void) *proc;\n"
                "char *name;\n"
                "i32 name_len;\n"
                "char *description;\n"
                "i32 description_len;\n"
                "char *source_name;\n"
                "i32 source_name_len;\n"
                "i32 line_number;\n"
                "};\n");
        
        fprintf(out,
                "static Command_Metadata fcoder_metacmd_table[%d] = {\n",
                entry_arrays.doc_string_count);
        for (i32 i = 0; i < entry_count; ++i){
            Meta_Command_Entry *entry = entries[i];
            
            Temp_Memory temp = begin_temp(arena);
            
            // HACK(allen): We could just get these at the HEAD END of the process,
            // then we only have to do it once per file, and pass the lengths through.
            //umem source_name_len = cstring_length(entry->source_name);
            String_Const_u8 source_name = SCu8(entry->source_name);
            String_Const_u8 printable = string_replace(arena, source_name,
                                                       SCu8("\\"), SCu8("\\\\"),
                                                       StringFill_NullTerminate);
            
            fprintf(out,
                    "{ PROC_LINKS(%.*s, 0), \"%.*s\", %d,  \"%.*s\", %d, \"%s\", %d, %lld },\n",
                    string_expand(entry->name),
                    string_expand(entry->name),
                    (i32)entry->name.size,
                    string_expand(entry->docstring.doc),
                    (i32)entry->docstring.doc.size,
                    printable.str,
                    (i32)source_name.size,
                    entry->line_number);
            end_temp(temp);
        }
        fprintf(out, "};\n");
        
        i32 id = 0;
        for (i32 i = 0; i < entry_count; ++i){
            Meta_Command_Entry *entry = entries[i];
            
            fprintf(out, "static i32 fcoder_metacmd_ID_%.*s = %d;\n",
                    string_expand(entry->name), id);
            ++id;
        }
        
        fprintf(out, "#endif\n");
        
        fclose(out);
    }
    else{
        fprintf(stdout, "fatal error: could not open output file %s%s\n", out_directory, COMMAND_METADATA_OUT);
    }
    
    return(0);
}

// BOTTOM
