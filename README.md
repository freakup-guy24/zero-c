# ZERO C

**A focused Windows C programming studio.** ZERO C brings a clean, dark-red,
VS Code-inspired editing workspace together with a portable GCC compiler,
diagnostics, and one-click build-and-run.

## Get ZERO C

For Windows 10/11 x64, choose the package that suits you from the latest
[ZERO C GitHub Release](../../releases/latest): run the per-user installer, or
extract the portable ZIP to a writable folder and launch `ZERO-C.exe`. Neither
option needs administrator access. The portable edition keeps its settings in
a `Settings` folder beside the app; keep the bundled `GCC`, `C`,
`portable.mode`, and `zero-c.spec` items together. Your personal C files are
not part of either package.

> Release installers are not code-signed. Windows SmartScreen may show an
> unknown-publisher prompt. Only run installers obtained from this
> repository's Releases.

### Portable edition

Download `ZERO-C-Portable-1.0.4.zip` from [Releases](../../releases/latest),
extract the whole archive to a writable folder (or USB drive), then run
`ZERO-C.exe` inside the extracted folder. Keep the `GCC`, `C`, `portable.mode`,
and `zero-c.spec` items together beside the app. No installation or admin
access is required. Portable settings are saved in the local `Settings` folder; move or copy the
whole extracted folder to carry them with the app. ZERO C creates
`Documents\ZERO C` automatically as the default source workspace. New files
and the Open/Save dialogs start there; existing files opened elsewhere can
still be saved in their original locations.

## Quick start

1. Launch **ZERO C** from the Start Menu.
2. Double-click a file in the Explorer, or choose **Open** to select a C file.
3. Edit your code and press **Ctrl+S**.
4. Choose **Build**. GCC warnings and errors appear in the Build Output pane.
5. Choose **Run** to open the last successful build in its own console window.

Builds are saved next to the source with a `.GCC.exe` suffix, leaving existing
`.EXE` files untouched. The Explorer lists `.c`, `.h`, and `.cpp` files under
`Documents\ZERO C`, which ZERO C creates automatically. ZERO C edits one source file at a time.

## What's included

- Native Windows editor with a dark-red theme, file explorer, active-file tab,
  output panel, and resizable window.
- Open, edit, and save UTF-8 C/C++ source files.
- GCC build with compiler diagnostics shown in the output panel.
- Double-click a GCC diagnostic to open its source and jump to the reported line.
- Run the last successful build in a separate console for keyboard input.
- Auto-indent, bracket matching, search/replace, and per-file numbered bookmarks.
- Restores the last open file, caret location, and window size/position.
- ZERO C app icon and custom in-app logo.
- Reliable native control redraw in the dark theme.
- Portable MinGW-w64 GCC in both packages; no system-wide PATH changes.
- Compatibility functions for common Turbo C `getch()` and `clrscr()` calls.

ZERO C uses modern GCC; it is **not** Borland Turbo C. Turbo C graphics
libraries, DOS hardware access, and the original source-level debugger are
not included. Other compiler-specific features may need to be updated.

## Source and build

The repository contains ZERO C's application source, compatibility header,
build/installer configuration, and documentation. It does not contain the
developer's personal C practice programs, prebuilt executables, or local
compiler downloads. The installer and portable ZIP package the app, its single compatibility
header, required documentation, and the separately licensed GCC toolchain.

The source editor can be built with a compatible 64-bit MinGW-w64 GCC and
Inno Setup 7. See [docs/USER-GUIDE.md](./docs/USER-GUIDE.md) for setup and
build instructions. Release maintainers should use
[docs/RELEASE-CHECKLIST.md](./docs/RELEASE-CHECKLIST.md).
The bundled installer is distributed as a GitHub Release asset, not a
regular repository file (it exceeds GitHub's 100 MB single-file limit).

## Third-party notices

The installer bundles WinLibs GCC/MinGW-w64 and their dependencies. See
[THIRD-PARTY-NOTICES.txt](./THIRD-PARTY-NOTICES.txt) and the license/source
notices distributed with the installed toolchain.

## Project status

ZERO C is a small independent learning project. It is not affiliated with
Microsoft, Visual Studio Code, Borland, Embarcadero, GCC, MinGW-w64, or
WinLibs.
