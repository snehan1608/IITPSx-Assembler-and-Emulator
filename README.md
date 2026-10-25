# IITPSx Assembler & Emulator

A two-pass assembler (`code/asm.c`) and a stack-machine emulator (`code/emu.c`) for **IITPSx**, a 32-bit accumulator-stack architecture. Both tools are written in portable ISO C99 with no dependencies, and were built as the CS2102 mini project at IIT Patna.

```
  prog.asm ──► asm ──► prog.o  ──► emu ──► prog_emu.log
                 │                           (registers + memory dump,
                 ├──► prog.lst                or runtime error)
                 └──► prog.log
```

## Features

**Assembler**
- True two-pass design: forward references resolve correctly, and branch offsets are computed PC-relative.
- Full ISA support plus the `data` and `SET` directives.
- Decimal, octal (`077`) and hexadecimal (`0x1F`) operands.
- Clear diagnostics with line numbers: duplicate labels, unknown mnemonics, missing or unexpected operands, undefined labels, bad numbers, out-of-range operands.
- Warnings for unused labels and for `br -1` self-loops.
- No `.o` file is written if any error occurs.

**Emulator**
- Loads little-endian `.o` files into 65,536 words of simulated memory.
- Traps and logs: division by zero, `INT_MIN / -1`, illegal opcodes, out-of-bounds PC or memory access, invalid shift counts, and runaway programs (100,000-instruction limit).
- Always writes a final register state and a memory dump, even after a trap.

## Quick start

Run these from the project root:

```bash
# build (no warnings under -pedantic -Wall)
gcc -std=c99 -pedantic -Wall code/asm.c -o asm
gcc -std=c99 -pedantic -Wall code/emu.c -o emu

# assemble, then run the object file (not the .asm)
./asm Tests/bubblesort.asm
./emu Tests/bubblesort.o
```

On Windows, use `asm.exe` and `emu.exe` (prebuilt in the root), for example `.\asm.exe Tests\bubblesort.asm`.

Output files are written next to the input, so everything for `Tests/bubblesort.asm` lands in `Tests/`.

The assembler writes three files next to the source:

| File | Contents |
| :--- | :--- |
| `bubblesort.o` | Binary object file, one 32-bit little-endian word per instruction or `data` |
| `bubblesort.lst` | Listing: address, machine word and source for each line, inline errors/warnings, symbol table |
| `bubblesort.log` | Errors, warnings and the label table |

The emulator writes `bubblesort_emu.log`:

```
A=0 (0x00000000)  B=0 (0x00000000)  PC=46  SP=65536

Memory Dump
00000000 FFFFFB0A 00002E00 00000004 FFFFFF01
...
0000002C 0000050A 00000012 00000006 FFFFFFF8
00000030 00000004 0000000F 00000017 0000002A
```

Here the array at `0x2F` has been sorted in place: `-8, 4, 15, 23, 42, 99`.

## Architecture

| Item | Description |
| :--- | :--- |
| Word size | 32-bit signed |
| Memory | 65,536 words, word-addressed |
| `A`, `B` | Two-register evaluation stack. Loading into `A` pushes the old `A` into `B`. Binary ops compute `B op A` into `A`. |
| `PC` | Program counter, starts at `0`. Incremented *before* an instruction executes, so branch offsets are relative to the next instruction. |
| `SP` | Stack pointer, starts at `65536` (just past the top of memory, stack grows downward). Reserve locals with a negative `adj` before using `ldl`/`stl`. |

**Instruction encoding:** `[ operand : 24 bits | opcode : 8 bits ]`. The operand is signed, from `-8,388,608` to `16,777,215`. `data` words use the full 32 bits.

## Instruction set

| Mnemonic | Opcode | Operand | Semantics |
| :--- | :---: | :---: | :--- |
| `data` | – | value | Reserve one word initialised to `value` |
| `SET` | – | value | Give the label on the same line the value `value` |
| `ldc` | 0 | value | `B = A; A = value` |
| `adc` | 1 | value | `A += value` |
| `ldl` | 2 | offset | `B = A; A = mem[SP + offset]` |
| `stl` | 3 | offset | `mem[SP + offset] = A; A = B` |
| `ldnl` | 4 | offset | `A = mem[A + offset]` |
| `stnl` | 5 | offset | `mem[A + offset] = B` |
| `add` | 6 | – | `A = B + A` |
| `sub` | 7 | – | `A = B - A` |
| `shl` | 8 | – | `A = B << A` (0 ≤ A < 32) |
| `shr` | 9 | – | `A = B >> A` (arithmetic, 0 ≤ A < 32) |
| `adj` | 10 | value | `SP += value` |
| `a2sp` | 11 | – | `SP = A; A = B` |
| `sp2a` | 12 | – | `B = A; A = SP` |
| `call` | 13 | label | `B = A; A = PC; PC += offset` |
| `return` | 14 | – | `PC = A; A = B` |
| `brz` | 15 | label | `if A == 0: PC += offset` |
| `brlz` | 16 | label | `if A < 0: PC += offset` |
| `br` | 17 | label | `PC += offset` |
| `HALT` | 18 | – | Stop execution cleanly |
| `mul` | 19 | – | `A = B * A` |
| `div` | 20 | – | `A = B / A` (traps on `A == 0` and `INT_MIN / -1`) |

## Writing assembly

```asm
; sum = 3 + 2 + 1
            adj   -2            ; two locals: [0]=n, [1]=sum
            ldc   3
            stl   0             ; n = 3
            ldc   0
            stl   1             ; sum = 0
loop:       ldl   1
            ldl   0
            add
            stl   1             ; sum += n
            ldl   0
            adc   -1
            stl   0             ; n--
            ldl   0
            brz   done
            br    loop
done:       ldl   1
            ldc   result
            stnl  0             ; result = sum
            adj   2             ; release locals
            HALT
result:     data  0
```

- **Labels:** start with a letter, letters and digits only (no underscores), at most 31 characters, end with `:`.
- **Comments:** everything after `;`.
- **Terminate with `HALT`** and put `data` words *after* it, otherwise the CPU executes data as instructions.
- Running the example above leaves `6` in the last word of the memory dump (`result`).

## Tests

All programs are in [`Tests/`](Tests), each with its generated `.o`, `.lst`, `.log` and `_emu.log`.

| Program | Checks | Result |
| :--- | :--- | :--- |
| `bubblesort.asm` | In-place sort of 6 signed integers | `-8, 4, 15, 23, 42, 99` |
| `fact.asm` | Iterative factorial, 5! | `120` stored in `factres` |
| `mean.asm` | Array sum and integer mean of `{10, 25, 35, 40, 15}` | `25` stored in `result` |
| `mul_and_div.asm` | `mul` and `div` | `6 * 7 / 2 = 21` |
| `set.asm` | `SET` directive | Final `A = 141` (75 + 66) |
| `div_zero.asm` | Runtime trap | `division by zero` logged, clean stop |
| `inf_loop.asm` | `br -1` warning and runaway program | Assembler warning, then `instruction limit (100000) exceeded` |
| `error.asm`, `warnings.asm` | Assembler diagnostics | 10 errors each, `.o` suppressed |

To regenerate a test:

```bash
./asm Tests/fact.asm && ./emu Tests/fact.o
```

## Repository layout

```
IITPSx-Assembler-and-Emulator
├── code/          asm.c (two-pass assembler), emu.c (emulator)
├── Tests/         test programs and their generated .o, .lst, .log, _emu.log
├── asm.exe        prebuilt assembler (Windows)
├── emu.exe        prebuilt emulator (Windows)
├── claims.txt     submission summary
└── README.md
```

## Limitations

- Memory is word-addressed only; there are no byte operations.
- No macros, expressions in operands (`label+4`), or multi-file linking.

