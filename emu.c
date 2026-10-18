/*
  Name    : Snehan Choudavarapu
  Roll no : 2501CS03
  Declaration: I declare that this is my own work
 */

#include <stdio.h>
#include <string.h>
#include <limits.h>
#include <stdarg.h>

#define MEM_WORDS 65536
#define DEFAULT_MAX_STEPS 100000UL

typedef unsigned int Word; // for 32bit machine word

int memory[MEM_WORDS];
int regA = 0, regB = 0, regPC = 0, regSP = MEM_WORDS; // SP starts at the top of memory (stack grows downwards)
int currentInstructionAddress = 0;
unsigned long stepCount = 0;
long loadedWords = 0;
FILE *log_fp = NULL;
// loads the binary file(little endian(32bit))into memory array
int load_file(char *name)
{
    FILE *file = fopen(name, "rb");
    unsigned char bytes[4];
    int count = 0;

    if (file == NULL)
    {
        fprintf(stderr, "cannot open '%s'\n", name);
        return -1;
    }
    while (fread(bytes, 1, 4, file) == 4)
    {
        if (count >= MEM_WORDS)
        {
            fprintf(stderr, "program too large for memory\n");
            fclose(file);
            return -1;
        }
        memory[count++] = (int)((Word)bytes[0] | ((Word)bytes[1] << 8) | ((Word)bytes[2] << 16) | ((Word)bytes[3] << 24));
    }
    if (ferror(file) || !feof(file))
    {
        fprintf(stderr, "error reading '%s'\n", name);
        fclose(file);
        return -1;
    }
    fclose(file);
    return count;
}
/* Memory dump, four words per line, address first. */
void dump_memory(long count)
{
    fprintf(log_fp, "\nMemory Dump\n");
    for (long i = 0; i < count; i++)
    {
        if (i % 4 == 0)
        {
            fprintf(log_fp, "%s%08lX", i == 0 ? "" : "\n",(unsigned long) i);
        }
        fprintf(log_fp, " %08X", (Word)memory[i]);
    }
    fprintf(log_fp, "\n");
}
// reports errors
int fail(char *format, ...)
{
    va_list args;
    fprintf(log_fp, "Runtime Error at address %d (instruction %lu): ", currentInstructionAddress, stepCount);
    va_start(args, format);
    vfprintf(log_fp, format, args);
    va_end(args);
    fprintf(log_fp, "\n");
    return 1;
}
int valid_addr(int address)
{
    return address >= 0 && address < MEM_WORDS;
}
/* Two's complement addition that wraps instead of overflowing. */
int add32(int x, int y)
{
    return (int)((Word)x + (Word)y);
}
int sign_ext(Word value)
{
    return (int)((value ^ 0x800000UL) - 0x800000UL);
}
/* returns 0 on HALT and 1 on a runtime error*/
int run(unsigned long maxSteps)
{
    Word instruction, opcode;
    int operand, address;
    for (;;)
    {
        currentInstructionAddress = regPC;
        if (!valid_addr(regPC))
        {
            return fail("PC %d is outside memory", regPC);
        }
        if (stepCount >= maxSteps)
        {
            return fail("instruction limit (%lu) exceeded-""infinite loop",maxSteps);
        }
        instruction = (Word)memory[regPC];
        regPC++; /* PC moves on before the action */
        stepCount++;
        opcode = instruction & 0xFF;
        operand = sign_ext(instruction >> 8);

        switch (opcode)
        {
        case 0: /* ldc */
            regB = regA;
            regA = operand;
            break;
        case 1: /* adc */
            regA = add32(regA, operand);
            break;
        case 2: /* ldl */
            address = add32(regSP, operand);
            if (!valid_addr(address))
            {
                return fail("ldl address %d outside memory", address);
            }
            regB = regA;
            regA = memory[address];
            break;
        case 3: /* stl */
            address = add32(regSP, operand);
            if (!valid_addr(address))
            {
                return fail("stl address %d outside memory", address);
            }
            memory[address] = regA;
            regA = regB;
            break;
        case 4: /* ldnl */
            address = add32(regA, operand);
            if (!valid_addr(address))
            {
                return fail("ldnl address %d outside memory", address);
            }
            regA = memory[address];
            break;
        case 5: /* stnl */
            address = add32(regA, operand);
            if (!valid_addr(address))
            {
                return fail("stnl address %d outside memory", address);
            }
            memory[address] = regB;
            break;
        case 6: /* add */
            regA = add32(regB, regA);
            break;
        case 7: /* sub */
            regA = (int)((Word)regB - (Word)regA);
            break;
        case 8: /* shl */
            if (regA < 0 || regA >= 32)
            {
                return fail("shl by invalid count %d", regA);
            }
            regA = (int)((Word)regB << regA);
            break;
        case 9: /* shr */
            if (regA < 0 || regA >= 32)
            {
                return fail("shr by invalid count %d", regA);
            }
            /* arithmetic shift, written so it is portable */
            regA = (regB >= 0) ? (regB >> regA) : ~((~regB) >> regA);
            break;
        case 10: /* adj */
            regSP = add32(regSP, operand);
            break;
        case 11: /* a2sp */
            regSP = regA;
            regA = regB;
            break;
        case 12: /* sp2a */
            regB = regA;
            regA = regSP;
            break;
        case 13: /* call */
            regB = regA;
            regA = regPC;
            regPC = add32(regPC, operand);
            break;
        case 14: /* return */
            regPC = regA;
            regA = regB;
            break;
        case 15: /* brz */
            if (regA == 0)
            {
                regPC = add32(regPC, operand);
            }
            break;
        case 16: /* brlz */
            if (regA < 0)
            {
                regPC = add32(regPC, operand);
            }
            break;
        case 17: /* br */
            regPC = add32(regPC, operand);
            break;
        case 18: /* HALT */
            return 0;
        case 19: /* mul */
            regA = (int)((Word)regB * (Word)regA);
            break;
        case 20: /* div */
            if (regA == 0)
            {
                return fail("division by zero (B=%d, A=0)", regB);
            }
            if (regB == INT_MIN && regA == -1)
            {
                return fail("division overflow (%d / -1)", regB);
            }
            regA = regB / regA;
            break;
        default:
            return fail("illegal opcode %u (instruction 0x%08X)", opcode, instruction);
        }
    }
}

void make_log_name(char *file_name, char *log_name)
{
    char *dot, *slash;
    strcpy(log_name, file_name);
    dot = strrchr(log_name, '.');
    slash = strrchr(log_name, '/');
    if (dot != NULL && (slash == NULL || dot > slash))
    {
        *dot = '\0';
    }
    strcat(log_name, "_emu.log");
}

int main(int argc, char *argv[])
{
    int status;
    char *file_name;
    char log_name[260];
    if (argc != 2)
    {
        printf("required as %s file.o\n", argv[0]);
        return 2;
    }
    file_name = argv[1];
    loadedWords = load_file(file_name); // load the .o binary file into memory array before running
    if (loadedWords < 0)
    {
        fclose(log_fp);
        return 2;
    }
    make_log_name(file_name, log_name); // making log file name
    log_fp = fopen(log_name, "w");
    if (log_fp == NULL)
    {
        fprintf(stderr, "cannot create '%s'\n", log_name);
        return 2;
    }
    status = run(DEFAULT_MAX_STEPS);
    fprintf(log_fp, "A=%d (0x%08X)  B=%d (0x%08X)  PC=%d  SP=%d\n", regA, (Word)regA, regB, (Word)regB, regPC, regSP); // prints a,b,pc,sp in log file
    dump_memory(loadedWords);
    fclose(log_fp);

    return status;
}