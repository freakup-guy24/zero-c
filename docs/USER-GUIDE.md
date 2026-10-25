# ZERO C user guide

## Install

Run `ZERO-C-Setup-1.0.4.exe` for a per-user installation, or download
`ZERO-C-Portable-1.0.4.zip` from the official release. For the portable
version, extract the complete ZIP to a writable folder or USB drive and launch
`ZERO-C.exe` inside it. Keep the GCC and C folders, `portable.mode`, and
`zero-c.spec` beside the application. No installation or admin access is
needed. Portable settings are saved in the local `Settings` folder, so move the
whole extracted folder to carry settings. Your personal C files are not part of
the download.

For a standard installation, administrator access is not required and you may
select the optional desktop shortcut during setup.

## Open and edit source

- Double-click a `.c`, `.h`, or `.cpp` file in the Explorer to open it.
- Use **Open** to choose a source file anywhere on your PC.
- The editor works with one open source file at a time.
- Press **Ctrl+S** to save. A dot beside the filename means it has unsaved
  edits. **New** starts a blank file.
- Press **Tab** to indent by four spaces. Press **Enter** to keep the previous
  line's indentation; a line ending in `{` adds one indentation level.
- Press **Ctrl+M** with the caret beside a bracket to select its matching
  `()`, `{}`, or `[]` bracket.
- Press **Ctrl+K**, then a digit `0`-`9` to set a bookmark. Press **Ctrl+Q**,
  then the same digit to return to it.

ZERO C automatically creates `Documents\ZERO C` as its default source folder
on first launch. The Explorer lists source files there, and New/Save/Open
dialogs start there. Files opened from other locations remain supported and
Ctrl+S saves them back to their existing path; use **Save As** to place a copy
in the Documents workspace. ZERO C does not move your existing source files. ZERO C remembers the last open file,
caret location, and window size/position between launches. Bookmarks are saved
per source file.

## Search and replace

- Press **Ctrl+F** to open Find.
- Press **Ctrl+H** to open Find and Replace.
- Use **Find Next**, **Replace**, or **Replace All** in the standard Windows
  dialog. Searches apply to the open source file.

## Build and find errors

Choose **Build**. ZERO C saves pending edits and asks bundled GCC to compile
the open file. Look at **Build Output** at the bottom:

- `file.c:line:column: error:` identifies an error GCC cannot compile.
- `file.c:line:column: warning:` identifies code GCC compiled but thinks may
  be incorrect.
- On failure, correct the source and choose **Build** again.
- Double-click a GCC error or warning line in Build Output to open that source
  file and jump to its reported line and column.

Build products use the `.GCC.exe` suffix so an existing Turbo C executable is
not overwritten.

## Run

After a successful build, choose **Run**. The program opens in its own console
window. Programs that call `getch()` can wait for a key there. Close the
program window when finished.

If you edit the source after building, build again before running so you are
using the latest code. The output is a Windows executable built with GCC, not
a DOS/Turbo C binary.

## Turbo C compatibility

ZERO C provides basic `getch()` and `clrscr()` compatibility for simple
console exercises. `conio.h` is supplied from the app's local compatibility
folder when compiling. Turbo C graphics (`graphics.h`), `void main()`, and
other DOS-specific APIs are not fully compatible with modern GCC. Compiler
diagnostics identify many issues, but some programs need manual porting.
ZERO C does not provide Turbo C's original source-level debugger, memory
models, DOS interrupts, or BGI graphics runtime.

## Display and rendering

Version 1.0.1 fixed dark-theme repaint issues in the editor and Build Output.
Version 1.0.2 fixed GCC linker failures when the install path contains spaces.
Version 1.0.3 added a portable ZIP edition with settings stored beside the app.
Version 1.0.4 creates a default `Documents\ZERO C` workspace for source files.
The launcher passes the bundled manifest object by library name, avoiding a
space-sensitive absolute object path.

## Troubleshooting

### “Could not start GCC”

Repair/reinstall ZERO C and check that `GCC\mingw64\bin\gcc.exe` exists in
the selected install directory.

### Build fails with `cannot find ... default-manifest.o`

This linker issue is fixed in v1.0.2. Run the v1.0.2 installer over the
existing installation; you do not need to uninstall or move your source files.

### My program needs input

Use **Run**; program input is handled in the separate console window.

### Windows says the publisher is unknown

The first release is unsigned. Only run a copy downloaded from the project's
official Releases page, and keep Windows security protections enabled.

### My old `.EXE` disappeared

ZERO C names newly compiled files `.GCC.exe`; it does not intentionally
overwrite existing `.EXE` files. Check the source file's directory.

## Keyboard shortcuts

| Shortcut | Action |
| --- | --- |
| `Ctrl+O` | Open a source file |
| `Ctrl+S` | Save the current file |
| `Ctrl+F` | Find |
| `Ctrl+H` | Find and replace |
| `Ctrl+M` | Match the bracket beside the caret |
| `Ctrl+K`, then `0`-`9` | Set a numbered bookmark |
| `Ctrl+Q`, then `0`-`9` | Jump to a numbered bookmark |
| `Tab` | Insert four spaces |
