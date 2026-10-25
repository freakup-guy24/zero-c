# ZERO C user guide

## Install

Run `ZERO-C-Setup-1.0.0.exe`, choose the install folder, and finish setup. Administrator access is not required. You can select the optional desktop shortcut during setup. Remove ZERO C from Windows **Settings → Apps**.

## Open and edit source

- Double-click a `.c`, `.h`, or `.cpp` file in the Explorer to open it.
- Use **Open** to choose a source file anywhere on your PC.
- The editor works with one open source file at a time.
- Press **Ctrl+S** to save. A dot beside the filename means it has unsaved edits. **New** starts a blank file.
- Press **Tab** to indent by four spaces.

The Explorer lists source files under the installed `C\TC` folder. Files in other folders remain accessible using **Open**. ZERO C does not copy or relocate your source files automatically.

## Build and find errors

Choose **Build**. ZERO C saves pending edits and asks bundled GCC to compile the open file. Look at **Build Output** at the bottom:

- `file.c:line:column: error:` identifies an error GCC cannot compile.
- `file.c:line:column: warning:` identifies code GCC compiled but thinks may be incorrect.
- On failure, correct the source and choose **Build** again.

The editor displays compiler messages but does not automatically highlight the reported source line. Build products use the `.GCC.exe` suffix so an existing Turbo C executable is not overwritten.

## Run

After a successful build, choose **Run**. The program opens in its own console window. Programs that call `getch()` can wait for a key there. Close the program window when finished.

If you edit the source after building, build again before running so you are using the latest code. The output is a Windows executable built with GCC, not a DOS/Turbo C binary.

## Turbo C compatibility

ZERO C provides basic `getch()` and `clrscr()` compatibility for simple console exercises. `conio.h` is supplied from the app's local compatibility folder when compiling. Turbo C graphics (`graphics.h`), `void main()`, and other DOS-specific APIs are not fully compatible with modern GCC. Compiler diagnostics identify many issues, but some programs need manual porting.

## Troubleshooting

### “Could not start GCC”

Repair/reinstall ZERO C and check that `GCC\mingw64\bin\gcc.exe` exists in the selected install directory.

### My program needs input

Use **Run**; program input is handled in the separate console window.

### Windows says the publisher is unknown

The first release is unsigned. Only run a copy downloaded from the project's official Releases page, and keep Windows security protections enabled.

### My old `.EXE` disappeared

ZERO C names newly compiled files `.GCC.exe`; it does not intentionally overwrite existing `.EXE` files. Check the source file's directory.

## Keyboard shortcuts

| Shortcut | Action |
| --- | --- |
| `Ctrl+O` | Open a source file |
| `Ctrl+S` | Save the current file |
| `Tab` | Insert four spaces |
