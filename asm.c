/*
  Name    : Snehan Choudavarapu
  Roll no : 2501CS03
  Declaration: I declare that this is my own work
*/
#include <ctype.h>
#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_LINE 512
#define MAX_LABELS 2048
#define MAX_LABEL_LEN 31
#define MAX_CODE_WORDS 65536
#define MAX_FILE_NAME 256
#define OPERAND_MIN (-8388608L)
#define OPERAND_MAX (16777215L)
#define MASK24 0xFFFFFFUL
#define MASK32 0xFFFFFFFFUL

typedef unsigned long Word; // for 32bit machine word
typedef enum
{
    none,
    value,
    branch
} Format; /*tells what an operand is(type of operand)*/
typedef enum
{
    instruction,
    data,
    set
} Kind; /*Tells what are the type commands*/
typedef struct
{ /*[24,8] [operand/value/immediate,opcode]*/
    char *name;
    int opcode;
    Format format; /* branch: label operand is pc-relative*/
    Kind kind;
} Instruction;
Instruction instruction_table[] = {
    {"data", -1, value, data},
    {"SET", -1, value, set},
    {"ldc", 0, value, instruction},
    {"adc", 1, value, instruction},
    {"ldl", 2, value, instruction},
    {"stl", 3, value, instruction},
    {"ldnl", 4, value, instruction},
    {"stnl", 5, value, instruction},
    {"add", 6, none, instruction},
    {"sub", 7, none, instruction},
    {"shl", 8, none, instruction},
    {"shr", 9, none, instruction},
    {"adj", 10, value, instruction},
    {"a2sp", 11, none, instruction},
    {"sp2a", 12, none, instruction},
    {"call", 13, branch, instruction},
    {"return", 14, none, instruction},
    {"brz", 15, branch, instruction},
    {"brlz", 16, branch, instruction},
    {"br", 17, branch, instruction},
    {"HALT", 18, none, instruction},
    {"mul", 19, none, instruction},
    {"div", 20, none, instruction}};
#define tab_size (sizeof(instruction_table) / sizeof(instruction_table[0]))
typedef struct
{
    char name[MAX_LABEL_LEN + 1];
    long value; /* address or the number given to SET */
    int is_set;
    int used;
    int line;
} Label;

Label labels[MAX_LABELS];//For symbol table
int label_count = 0;
Word code[MAX_CODE_WORDS]; /* assembled program */
long pc = 0;               /* current address */
char *source_name = "";
FILE *list_fp = NULL;
FILE *log_fp = NULL;
int current_pass = 1;
int line_no = 0;
int error_count = 0;
int warning_count = 0;
char inline_error[2100];
char inline_warning[2100];
char pending_label_warn[1024];
int list_label_warnings = 0;
/* label names used as operands (filled in pass 1) */
char referenced[MAX_LABELS][MAX_LABEL_LEN + 1];
int referenced_count = 0;
/* Writes file:line:type:message to the log and counts errors/warnings.
   Only the pass given is logged; the other pass just saves the text for the listing line. */
void report(int pass, char *type, char *format, ...)
{
    char message[2048];
    va_list args;
    int is_error = (strcmp(type, "error") == 0);

    va_start(args, format);
    vsprintf(message, format, args);
    va_end(args);

    if (pass == current_pass)
    {
        fprintf(log_fp, "%s:%d: %s: %s\n", source_name, line_no, type, message);
        if (is_error)
        {
            error_count++;
        }
        else
        {
            warning_count++;
        }
    }
    if (current_pass == 2)
    { /* keep the text in string for the listing */
        if (is_error && inline_error[0] == '\0')
        {
            sprintf(inline_error, "ERROR: Line %d: %s", line_no, message);
        }
        else if (!is_error && pass == 2 && inline_warning[0] == '\0')
        {
            sprintf(inline_warning, "WARN: Line %d: %s", line_no, message);
        }
    }
}
int strcmp_ignore_case(char *a, char *b)
{
    while (*a != '\0' && *b != '\0')
    {
        if (tolower((unsigned char)*a) != tolower((unsigned char)*b))
        {
            return 0;
        }
        a++;
        b++;
    }
    return *a == *b;
}
Instruction *find_instruction(char *name)
{
    size_t i;

    for (i = 0; i < tab_size; i++)
    {
        if (strcmp_ignore_case(name, instruction_table[i].name))
        {
            return &instruction_table[i];
        }
    }
    return NULL;
}
/* checks if our name is alphanumeric or not */
int valid_label_name(char *name)
{
    size_t i;

    if (!isalpha((unsigned char)name[0]) || strlen(name) > MAX_LABEL_LEN)
    {
        return 0;
    }
    for (i = 1; name[i] != '\0'; i++)
    {
        if (!isalnum((unsigned char)name[i]))
        {
            return 0;
        }
    }
    return 1;
}
/*Returns idx of label if it is present else -1*/
int find_label(char *name)
{
    int i;

    for (i = 0; i < label_count; i++)
    {
        if (strcmp(labels[i].name, name) == 0)
        {
            return i;
        }
    }
    return -1;
}
/* pass 1: remember that a label name is used as an operand,used later to find unused labels */
void note_reference(char *name)
{
    for (int i = 0; i < referenced_count; i++)
    {
        if (strcmp(referenced[i], name) == 0)
        {
            return;
        }
    }
    if (referenced_count < MAX_LABELS)
    {
        strcpy(referenced[referenced_count++], name);
    }
}
/* called between the passes: set used for every label that is referenced. */
void mark_used_labels(void)
{
    for (int i = 0; i < label_count; i++)
    {
        for (int j = 0; j < referenced_count; j++)
        {
            if (strcmp(labels[i].name, referenced[j]) == 0)
            {
                labels[i].used = 1;
                break;
            }
        }
    }
}
/*Adds label in table and checks if it exists or not and give related warnings*/
void place_label(char *name, int valid, long label_value, int is_set)
{
    char warning[MAX_LABEL_LEN + 64];
    int existing;
    if (!valid)
    {
        return;
    }
    existing = find_label(name);
    if (current_pass == 1)
    { /*In pass 1= enter the label in the table,give duplicate label warning if needed*/
        if (existing >= 0)
        {
            report(1, "error", "duplicate label '%s' (first on line %d)", name, labels[existing].line);
        }
        else
        {
            strcpy(labels[label_count].name, name);
            labels[label_count].value = label_value;
            labels[label_count].is_set = is_set;
            labels[label_count].used = 0;
            labels[label_count].line = line_no;
            label_count++;
        }
        return;
    }
    else
    { /* In pass 2= re check for duplicate(then error shown in .lst) and unused label  warning*/
        if (existing >= 0 && labels[existing].line != line_no)
        {
            report(1, "error", "duplicate label '%s' (first on line %d)", name, labels[existing].line);
        }

        if (list_label_warnings && !is_set && existing >= 0 && labels[existing].line == line_no && !labels[existing].used)
        {
            sprintf(warning, "WARN: Label '%s' defined but never used", name);
            if (strlen(pending_label_warn) + strlen(warning) + 3 < sizeof pending_label_warn)
            {
                if (pending_label_warn[0] != '\0')
                {
                    strcat(pending_label_warn, "  ");
                }
                strcat(pending_label_warn, warning);
            }
        }
    }
}
/* changes string to long int (string can be decimal,oct,hex) */
int str_to_long(char *text, long *number)
{
    char *digits = text;
    char *end;
    long result;
    if (*digits == '+' || *digits == '-')
    { /* +ve or -ve if mentioned*/
        digits++;
    }
    if (!isdigit((unsigned char)*digits))
    {
        return 0;
    }
    errno = 0;
    result = strtol(text, &end, 0); /*it can directly change dec,oct,hex(inbuilt) to long int*/
    if (errno != 0 || *end != '\0')
    {
        return 0;
    }
    *number = result;
    return 1;
}
/* returns 1 if value is in range,24 bits-instructions, 32 bits-data. */
int in_range(long number, int is_data)
{
    if (is_data)
        return number >= -2147483647L - 1 && (number <= 0x7FFFFFFFL || (unsigned long)number <= MASK32);
    return number >= OPERAND_MIN && number <= OPERAND_MAX;
}
/*find the operand string to int,(pc-relative for branch),return 1 on success else 0 */
int evaluate_operand(char *token, int is_branch, int is_data, long *number, int *label_index)
{
    int index;
    *number = 0;
    *label_index = -1;
    //If it starts with letter then its a label else a number
    if (isalpha((unsigned char)token[0]))
    {
        if (!valid_label_name(token))
        {
            report(1, "error", "bad label name '%s' used as operand", token);
            return 0;
        }
        if (current_pass == 1)
        {
            note_reference(token); /* value not needed yet bcz we are just making table */
            return 1;
        }
        index = find_label(token);
        if (index < 0)
        {
            report(2, "error", "no such label '%s'", token);
            return 0;
        }
        labels[index].used = 1;
        *label_index = index;
        *number = labels[index].value;
        if (is_branch)
        {
            *number -= (pc + 1); /* pc is incremented before executing */
        }

        return 1;
    }
    //Checking things for a number
    if (!str_to_long(token, number))
    {
        *number = 0;
        report(1, "error", "'%s' is not a valid number or label", token);
        return 0;
    }

    if (!in_range(*number, is_data))
    {
        report(1, "error", "operand '%s' is out of range", token);
        *number = 0;
        return 0;
    }

    return 1;
}
/* Copy the word into the out array without taking the initial spaces in it by using cursor*/
char *read_token(char *cursor, char *out)
{
    while (isspace((unsigned char)*cursor))
    {
        cursor++;
    }
    while (*cursor != '\0' && !isspace((unsigned char)*cursor))
    {
        *out++ = *cursor++;
    }
    *out = '\0';
    return cursor;
}
/* Every line: address|machine code|code exactly as written in that line. */
void write_listing(long address, Word word, char *text)
{
    if (list_fp == NULL)
    {
        return;
    }
    fprintf(list_fp, "%08lX %08lX %s", (unsigned long)address, (unsigned long)word, text);
    if (inline_error[0] != '\0')
    {
        fprintf(list_fp, "  %s", inline_error);
    }
    if (pending_label_warn[0] != '\0')
    {
        fprintf(list_fp, "  %s", pending_label_warn);
        pending_label_warn[0] = '\0';
    }
    if (inline_warning[0] != '\0')
    {
        fprintf(list_fp, "  %s", inline_warning);
    }
    fputc('\n', list_fp);
}
/* Give the current line one memory word (pass 2 also stores and lists it). */
void emit_word(Word word, char *text)
{
    if (pc >= MAX_CODE_WORDS)
    {
        report(1, "error", "program too large (limit %d words)", MAX_CODE_WORDS);
        return;
    }

    if (current_pass == 2)
    {
        code[pc] = word;
        write_listing(pc, word, text);
    }

    pc++;
}
/*Process one line*/
void process_line(char *text)
{
    char label_name[MAX_LINE], mnemonic[MAX_LINE];
    char operand_text[MAX_LINE], extra_text[MAX_LINE];
    char rest_text[MAX_LINE], statement_text[2 * MAX_LINE + 2];
    char *cursor, *start, *semicolon;
    Instruction *entry;
    int has_label = 0, label_valid = 0, operand_ok, label_index;
    size_t len;
    long number;
    Word word;

    inline_error[0] = '\0'; /* this stores only the error,warning in this line */
    inline_warning[0] = '\0';

    semicolon = strchr(text, ';'); /* find semicolon and replace it with null char to not read comment*/
    if (semicolon != NULL)
    {
        *semicolon = '\0';
    }
    cursor = text;
    while (isspace((unsigned char)*cursor))
    {
        cursor++;
    }
    if (*cursor == '\0')
    {
        return; /* if this line has comment only */
    }

    /* Optional label: a first word that ends in ':' */
    start = cursor;
    while (*cursor != '\0' && *cursor != ':' && !isspace((unsigned char)*cursor))
    {
        cursor++;
    }
    if (*cursor == ':')
    {
        len = (size_t)(cursor - start);
        memcpy(label_name, start, len);
        label_name[len] = '\0';
        has_label = 1;
        cursor++;
        label_valid = valid_label_name(label_name);
        if (!label_valid)
        {
            report(1, "error", "Invalid label name %s", label_name);
        }
    }
    else
    {
        cursor = start; /* no label: rescan as mnemonic(means things like add,set etc) */
    }

    cursor = read_token(cursor, mnemonic);//reads mnemonic into mnemonic array
    if (mnemonic[0] == '\0')
    { /* label on its own */
        place_label(label_name, label_valid, pc, 0);
        return;
    }
    /* statement text for the listing: mnemonic and operand as written */
    start = cursor;
    while (isspace((unsigned char)*start))
    {
        start++;
    }
    strcpy(rest_text, start);
    len = strlen(rest_text);
    while (len > 0 && isspace((unsigned char)rest_text[len - 1]))
    {
        rest_text[--len] = '\0';
    }
    sprintf(statement_text, "%s%s%s", mnemonic, rest_text[0] != '\0' ? " " : "", rest_text);

    entry = find_instruction(mnemonic);
    if (entry == NULL)
    {//unknown mnemonic
        report(1, "error", "unknown mnemonic '%s'", mnemonic);
        if (has_label)
        {
            place_label(label_name, label_valid, pc, 0);
        }
        emit_word(0, statement_text); /*word is emmitted anyway because later address dont /shouldnt shift */
        return;
    }

    if (has_label && entry->kind != set)
    {
        place_label(label_name, label_valid, pc, 0);
    }

    cursor = read_token(cursor, operand_text);//read operand
    cursor = read_token(cursor, extra_text);//if not empty it is error
    if (extra_text[0] != '\0')
    {
        report(1, "error", "extra text at end of line ('%s')", extra_text);
    }

    number = 0;
    label_index = -1;
    operand_ok = 1;
    if (entry->format == none)
    {
        if (operand_text[0] != '\0')
        {
            report(1, "error", "unexpected operand '%s' for '%s'", operand_text, entry->name);
            (void)str_to_long(operand_text, &number); /* number stays in word */
        }
    }
    else if (operand_text[0] == '\0')
    {
        report(1, "error", "missing operand for '%s'", entry->name);
        operand_ok = 0;
    }
    else if (entry->kind == set)
    {
        if (!str_to_long(operand_text, &number) || !in_range(number, 1))
        {
            report(1, "error", "SET needs a valid numeric value, given '%s'", operand_text);
            number = 0;
        }
    }
    else
    {//computes the number
        operand_ok = evaluate_operand(operand_text, entry->format == branch, entry->kind == data, &number, &label_index);
    }

    if (entry->kind == set)
    { /* defines a label */
        if (!has_label)
        {
            report(1, "error", "SET needs a label on the same line");
        }
        else
        { /*number is made as the given offset or mem adress in evaluate_operand*/
            place_label(label_name, label_valid, number, 1);
        }
        return;
    }
    word = 0;
    if (current_pass == 2)
    { /*make the 32bit machine code*/
        if (entry->kind == data)
        {
            word = (Word)number & MASK32;
        }
        else
        {
            word = ((((Word)number) & MASK24) << 8) | (Word)entry->opcode;
        }

        if (entry->kind == instruction && entry->opcode == 17 && number == -1)
        {
            report(2, "warning", "possible infinite loop (backward jump to itself)");
        }
        (void)operand_ok;
    }
    emit_word(word, statement_text);
}

void run_pass(FILE *src, int pass)
{
    char text[MAX_LINE];
    int len;
    int c;
    rewind(src); /*starts from file begining*/
    /*initializing everythin for new pass*/
    current_pass = pass;
    pc = 0;
    line_no = 0;
    pending_label_warn[0] = '\0';
    list_label_warnings = (pass == 2 && error_count == 0);
    while (fgets(text, sizeof text, src) != NULL)
    {
        line_no++;
        len = (int)strlen(text);
        if (len > 0 && text[len - 1] != '\n' && !feof(src))
        {
            report(1, "error", "line too long (limit %d characters)", MAX_LINE - 1);
            while ((c = getc(src)) != EOF && c != '\n')
            {
            }
            continue;
        }
        process_line(text);
    }
}
/* .o file - each word as 4 bytes, least significant first. */
int write_object_file(char *path)
{
    FILE *fp = fopen(path, "wb");
    unsigned char bytes[4];
    long i;

    if (fp == NULL)
    {
        return 0;
    }
    for (i = 0; i < pc; i++)
    {
        bytes[0] = (unsigned char)(code[i] & 0xFF);
        bytes[1] = (unsigned char)((code[i] >> 8) & 0xFF);
        bytes[2] = (unsigned char)((code[i] >> 16) & 0xFF);
        bytes[3] = (unsigned char)((code[i] >> 24) & 0xFF);
        if (fwrite(bytes, 1, 4, fp) != 4)
        {
            fclose(fp);
            return 0;
        }
    }
    return fclose(fp) == 0;
}
void write_symbol_table(FILE *fp)
{
    fprintf(fp, "\n--- labels ---\n");
    for (int i = 0; i < label_count; i++)
    {
        fprintf(fp, "%s : %ld\n", labels[i].name, labels[i].value);
    }
}

int main(int argc, char *argv[])
{
    char base[MAX_FILE_NAME], obj_name[MAX_FILE_NAME], lst_name[MAX_FILE_NAME];
    char log_name[MAX_FILE_NAME];
    char *dot, *slash;
    FILE *src;
    if (argc != 2)
    {
        printf("Input is not in required format");
        return 2;
    }
    src = fopen(argv[1], "r");
    if (src == NULL)
    {
        fprintf(stderr, "cannot open '%s'\n", argv[1]);
        return 2;
    }
    /*taking name for creating .lst .log .o files*/
    source_name = argv[1];
    strcpy(base, argv[1]);
    dot = strrchr(base, '.');
    slash = strrchr(base, '/');
    if (dot != NULL && (slash == NULL || dot > slash))
    {
        *dot = '\0';
    }
    strcpy(obj_name, base);
    strcat(obj_name, ".o");
    strcpy(lst_name, base);
    strcat(lst_name, ".lst");
    strcpy(log_name, base);
    strcat(log_name, ".log");
    log_fp = fopen(log_name, "w"); /* opened before pass 1 so it writes every error */
    if (log_fp == NULL)
    {
        fprintf(stderr, "cannot create %s\n", log_name);
        return 2;
    }

    run_pass(src, 1);
    mark_used_labels();

    list_fp = fopen(lst_name, "w");
    if (list_fp == NULL)
    {
        fprintf(log_fp, "cannot create %s\n", lst_name);
        fclose(log_fp);
        return 2;
    }

    run_pass(src, 2);

    write_symbol_table(list_fp);
    fclose(list_fp);
    list_fp = NULL;

    for (int i = 0; i < label_count; i++)
    {
        if (!labels[i].used)
        {
            line_no = labels[i].line;
            report(2, "warning", "label '%s' defined but never used", labels[i].name);
        }
    }
    
    fclose(src);
    if (error_count > 0)
    { /*If there are any errors the .o file isnt made else .o file is made*/
        remove(obj_name);
        fprintf(log_fp, " %d errors, .o file not created\n", error_count);
        printf("%d errors, .o file not created\n", error_count);
        write_symbol_table(log_fp);
        fclose(log_fp);
        return 1;
    }
    if (!write_object_file(obj_name))
    {
        fprintf(log_fp, "cannot create '%s'\n", obj_name);
        write_symbol_table(log_fp);
        fclose(log_fp);
        return 2;
    }
    fprintf(log_fp, "\n%s: %ld-pc, %d warnings,%s and %s created\n", argv[1], pc, warning_count, obj_name, lst_name);
    printf("\nProgram assembled\n");
    write_symbol_table(log_fp);
    fclose(log_fp);
    return 0;
}