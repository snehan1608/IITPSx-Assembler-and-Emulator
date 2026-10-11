# IITPSx Assembler and Emulator

CS2102 mini project. A two-pass assembler (`asm.c`) and an emulator (`emu.c`) for the IITPSx processor, written in C.

## Build and run

```
gcc -std=c99 -pedantic asm.c -o asm
./asm test.asm

gcc -std=c99 -pedantic emu.c -o emu
./emu test.o
```

Run the emulator only if the assembler created the `.o` file.

## Assembler

`./asm file.asm` prints nothing to the terminal except "cannot open" messages. It writes three files next to the source:

- `file.o` - binary object file (not created if there are errors)
- `file.lst` - listing with address, machine code and the original line
- `file.log` - errors, warnings and a summary

The `.lst` and `.log` files both end with the label table:

```
--- labels ---
loop : 3
var1 : 7
```

It reports duplicate labels, missing labels, bad label names, unknown mnemonics, missing or unexpected operands and invalid numbers. It warns about unused labels and branches to itself. It supports `data`, `SET`, and the extra `mul` (opcode 19) and `div` (opcode 20) instructions.

## Emulator

`./emu [-t] [-b] [-d] [-s maxsteps] file.o`

- `-t` trace each instruction
- `-b` / `-d` dump memory before / after running
- `-s` instruction limit for catching infinite loops

It stops with an error on an illegal opcode, an address outside memory, division by zero, a bad shift count, or when the instruction limit is hit.

## Tests

- `test1.asm` - valid program (labels, forward reference, data)
- `test2.asm` - assembly errors
- `test3.asm` - `SET`
- `test4.asm` - `mul` and `div`, leaves 21 in A