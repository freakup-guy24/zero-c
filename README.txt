ZERO C  |  Portable C Programming Studio
========================================

ZERO C is a focused Windows desktop editor for learning and writing C. It
keeps your source files as normal files and uses the bundled GCC toolchain.

GET STARTED
1. Open ZERO-C.exe.
2. Double-click a file in the Explorer, or use Open to browse to a C source.
3. Edit the file. Ctrl+S saves it; Ctrl+O opens another file.
4. Click Build. Warnings and compiler errors are shown in the Build Output pane.
5. Click Run to launch the last successful build in a separate console.

Build creates a .GCC.exe next to your source file; an existing .EXE is not
overwritten. Use New to start a file, then Save to select its name and folder.

EDITOR FEATURES
- One source file open at a time, with an Explorer list of C, header and C++
  files in the application's C\TC folder.
- Dark-red theme, app logo, fixed-width editing, automatic
  indentation, bracket matching, and search/replace.
- Numbered per-file bookmarks; the last file, cursor, and window size are
  restored the next time ZERO C starts.
- Double-click a GCC diagnostic to open its file and navigate to its location.
- Ctrl+O / Ctrl+S, plus a separate console for program input/output.
- The editor changes no Windows-wide environment settings.

COMPILER AND TURBO C NOTES
The bundled WinLibs GCC toolchain is a modern compiler, not Borland Turbo C.
A small compatibility header provides common getch() and clrscr() functions.
The original Turbo C debugger, DOS hardware/interrupt APIs, and graphics
runtime are not included; compiler-specific code may need porting.
Programs that need keyboard input should be run in the console opened by Run.

INSTALL
Run ZERO-C-Setup-1.0.1.exe. It installs for the current Windows user, creates
a Start Menu shortcut, and optionally creates a desktop shortcut. Uninstall
from Windows Settings > Apps. Your C source files are not removed by uninstall.

BUILD THE APP AND INSTALLER
Build-ZERO-C.ps1 takes the path to a WinLibs GCC/MinGW-w64 toolchain and the
path to Inno Setup 7's ISCC.exe. Example:

  powershell -ExecutionPolicy Bypass -File .\Build-ZERO-C.ps1 `
    -CompilerDirectory 'C:\path\to\mingw64' `
    -InnoCompiler 'C:\Program Files (x86)\Inno Setup 7\ISCC.exe'

Use a supported x64 WinLibs release with g++.exe and windres.exe. Build
outputs are placed in build\ and dist\. The installer includes the compiler
toolchain, so it is large. Review THIRD-PARTY-NOTICES.txt before redistributing.

The installer is not digitally signed. Windows may show an unknown-publisher
warning; do not disable security protections to bypass it.

KEYBOARD SHORTCUTS
Ctrl+F: Find     Ctrl+H: Replace     Ctrl+M: Match bracket
Ctrl+K then 0-9: Set bookmark        Ctrl+Q then 0-9: Jump to bookmark
