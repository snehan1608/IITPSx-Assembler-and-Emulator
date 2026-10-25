import tkinter as tk
from tkinter import filedialog, messagebox, scrolledtext, ttk
import os,subprocess,sys,tempfile

# asm and emu executables sit next to this file
HERE = os.path.dirname(os.path.abspath(__file__))
EXT = ".exe" if sys.platform.startswith("win") else ""
ASM = os.path.join(HERE, "asm" + EXT)
EMU = os.path.join(HERE, "emu" + EXT)


def show(box, text):
    box.config(state=tk.NORMAL)  # unlock, replace the text, lock again
    box.delete("1.0", tk.END)
    box.insert(tk.END, text)
    box.config(state=tk.DISABLED)

def read_file(path):
    if not os.path.exists(path):
        return ""
    with open(path, errors="replace") as f:
        return f.read()

def run(cmd, folder):
    result = subprocess.run(cmd, cwd=folder, capture_output=True, text=True, errors="replace")
    return result.stdout + result.stderr

def assemble_and_run():
    if not (os.path.exists(ASM) and os.path.exists(EMU)):
        messagebox.showerror("Missing files", "asm and emu executables not found next to app.py.\nCreate them.")
        return

    # temp folder keeps the project free of generated files
    with tempfile.TemporaryDirectory() as d:
        tmp = lambda name: os.path.join(d, name)
        with open(tmp("prog.asm"), "w") as f:
            f.write(code_editor.get("1.0", tk.END))

        console = run([ASM, "prog.asm"], d)  # file name only, so messages have no full path
        show(log_box, read_file(tmp("prog.log")) or console)  # exactly what asm wrote to its log
        show(lst_box, read_file(tmp("prog.lst")))

        if not os.path.exists(tmp("prog.o")):  # no .o file means assembly errors, skip the emulator
            show(emu_box, "Issue in code so .o file not created, fix error.")
            tabs.select(0)
            return

        console = run([EMU, "prog.o"], d)
        show(emu_box, read_file(tmp("prog_emu.log")) + "\n" + console)
        tabs.select(2)

def open_file():
    tests = os.path.join(HERE, "Tests")
    path = filedialog.askopenfilename(initialdir=tests if os.path.isdir(tests) else HERE,filetypes=[("Assembly", "*.asm"), ("All files", "*.*")])
    if path:
        code_editor.delete("1.0", tk.END)
        code_editor.insert(tk.END, read_file(path))

def save_file():
    path = filedialog.asksaveasfilename(defaultextension=".asm", filetypes=[("Assembly", "*.asm")])
    if path:
        with open(path, "w") as f:
            f.write(code_editor.get("1.0", tk.END))

def make_tab(title):
    box = scrolledtext.ScrolledText(tabs, font=("Consolas", 10), state=tk.DISABLED, bg="#7CC0E5", fg="black")
    tabs.add(box.frame, text=title)
    return box

#Creating the GUI 
Window = tk.Tk()
Window.title("IITPSx Assembler & Emulator")
Window.geometry("1100x700")
# top bar with buttons
top = tk.Frame(Window)
top.pack(fill=tk.X, padx=6, pady=6)
tk.Button(top, text="Open .asm", command=open_file).pack(side=tk.LEFT)
tk.Button(top, text="Save .asm", command=save_file).pack(side=tk.LEFT, padx=4)
tk.Button(top, text="Clear", command=lambda: code_editor.delete("1.0", tk.END)).pack(side=tk.LEFT, padx=4)
tk.Button(top, text="Assemble & Run", command=assemble_and_run, bg="#4caf50", fg="white").pack(side=tk.LEFT, padx=4)
# editor on the left, output tabs on the right
main = tk.PanedWindow(Window, orient=tk.HORIZONTAL)
main.pack(fill=tk.BOTH, expand=True, padx=6, pady=(0, 6))

left = tk.LabelFrame(main, text="Assembly source")
code_editor = scrolledtext.ScrolledText(left, font=("Consolas", 11), width=50, undo=True)
code_editor.pack(fill=tk.BOTH, expand=True)
code_editor.insert(tk.END, ";Write Code here")
main.add(left)

tabs = ttk.Notebook(main)
main.add(tabs)
log_box = make_tab("Log")
lst_box = make_tab("Listing")
emu_box = make_tab("Emulator output")

Window.mainloop()