# IITPSx Two-Pass Assembler & Emulator

A two-pass assembler (`asm.c`) and a stack-based machine emulator (`emu.c`) built in strict C99 for the **IITPSx** 32-bit accumulator-stack architecture.

---

## 1. Architecture Overview

The IITPSx processor operates on 32-bit signed words (up to 65,536 words of memory) using four internal 32-bit registers:
* **`regA` (Accumulator) & `regB` (Secondary Accumulator):** Configured as a two-element evaluation stack. Loading a new value into `A` pushes the existing value of `A` into `B`. Binary operations (like `add`, `sub`, `mul`, `div`) combine `B` and `A` and store the result in `A`.
* **`regPC` (Program Counter):** Tracks the address of the next instruction to execute (initialized to `0`).
* **`regSP` (Stack Pointer):** Points to the current top of the local call frame in memory.

### Instruction Format
Each machine instruction is encoded as a 32-bit little-endian word:
* **Bits [31:8] (24 bits):** Signed immediate operand or PC-relative branch offset (`-8,388,608` to `16,777,215`).
* **Bits [7:0] (8 bits):** Instruction opcode.

---

## 2. Instruction Set Architecture (ISA)

| Mnemonic | Opcode | Operand Format | Operation / Semantics |
| :--- | :---: | :---: | :--- |
| `data` | — | `value` | Reserves one 32-bit memory word initialized to `value` |
| `SET` | — | `value` | Assigns `value` directly to the label on the same line |
| `ldc` | `0` | `value` | `B = A; A = value` |
| `adc` | `1` | `value` | `A = A + value` |
| `ldl` | `2` | `offset` | `B = A; A = memory[SP + offset]` |
| `stl` | `3` | `offset` | `memory[SP + offset] = A; A = B` |
| `ldnl` | `4` | `offset` | `A = memory[A + offset]` |
| `stnl` | `5` | `offset` | `memory[A + offset] = B` |
| `add` | `6` | `none` | `A = B + A` |
| `sub` | `7` | `none` | `A = B - A` |
| `shl` | `8` | `none` | `A = B << A` (0 <= A < 32) |
| `shr` | `9` | `none` | `A = B >> A` (arithmetic shift, 0 <= A < 32) |
| `adj` | `10` | `value` | `SP = SP + value` |
| `a2sp` | `11` | `none` | `SP = A; A = B` |
| `sp2a` | `12` | `none` | `B = A; A = SP` |
| `call` | `13` | `branch` | `B = A; A = PC; PC = PC + offset` |
| `return` | `14` | `none` | `PC = A; A = B` |
| `brz` | `15` | `branch` | `if (A == 0) PC = PC + offset` |
| `brlz` | `16` | `branch` | `if (A < 0) PC = PC + offset` |
| `br` | `17` | `branch` | `PC = PC + offset` |
| `HALT` | `18` | `none` | Stops emulator execution cleanly |
| `mul` | `19` | `none` | `A = B * A` |
| `div` | `20` | `none` | `A = B / A` (traps division by zero and `INT_MIN / -1`) |

---

## 3. Repository Structure

* **`asm.c`** — Two-pass assembler. Parses `.asm` source files, builds the symbol table, flags syntax/semantic errors and warnings, and outputs `.o`, `.lst`, and `.log` files.
* **`emu.c`** — IITPSx emulator. Loads `.o` binaries into memory, executes instructions with runtime safety bounds checking, and outputs register states and memory dumps to `<name>_emu.log`.
* **`claims.txt`** — Verification summary and testing claims.
* **Test Programs (`*.asm`):**
  * `bubblesort.asm` — In-place ascending Bubble Sort of a signed integer array.
  * `fact.asm` — Iterative factorial ($N!$) calculation.
  * `mean.asm` — Array sum and integer average calculation.
  * `mul_and_div.asm` — Arithmetic test for `mul` and `div` instructions.
  * `set.asm` — Verification of the `SET` pseudo-instruction.
  * `div_zero.asm` — Runtime trap verification for division by zero.
  * `inf_loop.asm` — Assembler warning verification for `br -1` self-loops.
  * `error.asm` & `warnings.asm` — Assembler diagnostic tests (duplicate labels, unknown mnemonics, unused labels, out-of-range operands).

---

## 4. How to Compile & Run

### Compilation
Both programs are written in strict ISO C99 and compile without warnings under `-pedantic` and `-Wall`:

```bash
gcc -std=c99 -pedantic -Wall asm.c -o asm
gcc -std=c99 -pedantic -Wall emu.c -o emu
```

### Step 1: Assemble a Program
Pass any `.asm` source file to the assembler:

```bash
./asm bubblesort.asm
```

This generates three files in the same directory:
1. **`bubblesort.o`** — 32-bit little-endian binary object file (suppressed if any assembly errors occur).
2. **`bubblesort.lst`** — Formatted listing file displaying hex addresses, assembled machine words, source statements, inline diagnostics, and the symbol table.
3. **`bubblesort.log`** — Diagnostic log containing pass errors, warnings, and symbol mappings.

### Step 2: Run the Emulator
Pass the compiled `.o` binary file (not the `.asm` file) to the emulator:

```bash
./emu bubblesort.o
```

This executes the binary and generates **`bubblesort_emu.log`**, which contains:
* Any runtime error diagnostics (if triggered).
* Final register state (`A`, `B`, `PC`, `SP`).
* A formatted hexadecimal memory dump (4 words per line) of the loaded program and data region.

---

## 5. Assembly Syntax & Guidelines

To write valid `.asm` programs for this toolchain, follow these rules:
1. **Labels:** Must start with an alphabetical character, contain only alphanumeric characters (max 31 chars), and end with a colon (`:`).
2. **Number Formats:** Immediate operands support decimal (`42`, `-8`), octal (`077`), and hexadecimal (`0x100`).
3. **Comments:** Anything following a semicolon (`;`) on a line is ignored.
4. **Always Terminate Execution:** End the control flow of every program with `HALT` before any `data` declarations so the processor does not execute data words or empty memory as `ldc 0` instructions.
5. **Runtime Trap Protection:** The emulator automatically detects and aborts on:
   * Division by zero or signed division overflow (`INT_MIN / -1`).
   * Out-of-bounds memory reads/writes (`ldl`, `stl`, `ldnl`, `stnl`) or out-of-bounds `PC`.
   * Invalid bit-shift counts (`< 0` or `>= 32`).
   * Illegal opcodes and infinite loops exceeding `100,000` steps.