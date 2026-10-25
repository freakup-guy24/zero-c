# ZERO C

**A focused Windows C programming studio.** ZERO C brings a clean, dark-red,
VS Code-inspired editing workspace together with a portable GCC compiler,
diagnostics, and one-click build-and-run.

## Get ZERO C

For Windows 10/11 x64, download and run the installer attached to the latest
[ZERO C GitHub Release](../../releases/latest). The installer is
per-user and does not need administrator privileges. It installs ZERO C and
its GCC toolchain, creates a Start Menu shortcut, and optionally creates a
desktop shortcut. The v1.0.2 release fixes GCC linking from the default install
folder, which contains a space. Run its installer over v1.0.1; your source
files are not part of the release package. Uninstall through **Settings → Apps → Installed apps**.

> Release installers are not code-signed. Windows SmartScreen may show an
> unknown-publisher prompt. Only run installers obtained from this
> repository's Releases.

## Quick start

1. Launch **ZERO C** from the Start Menu.
2. Double-click a file in the Explorer, or choose **Open** to select a C file.
3. Edit your code and press **Ctrl+S**.
4. Choose **Build**. GCC warnings and errors appear in the Build Output pane.
5. Choose **Run** to open the last successful build in its own console window.

Builds are saved next to the source with a `.GCC.exe` suffix, leaving existing
`.EXE` files untouched. The Explorer lists `.c`, `.h`, and `.cpp` files under
the installed `C\TC` folder. ZERO C edits one source file at a time.

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
- Portable MinGW-w64 GCC in the installer; no system-wide PATH changes.
- Compatibility functions for common Turbo C `getch()` and `clrscr()` calls.

ZERO C uses modern GCC; it is **not** Borland Turbo C. Turbo C graphics
libraries, DOS hardware access, and the original source-level debugger are
not included. Other compiler-specific features may need to be updated.

## Source and build

The repository contains ZERO C's application source, compatibility header,
build/installer configuration, and documentation. It does not contain the
developer's personal C practice programs, prebuilt executables, or local
compiler downloads. The installer packages the app, its single compatibility
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
