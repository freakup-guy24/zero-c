# ZERO C release checklist

- [ ] Review the application source and third-party notices.
- [ ] Use a clean, supported x64 WinLibs/MinGW-w64 GCC toolchain.
- [ ] Install Inno Setup 7 and verify its official installer signature.
- [ ] Build the app and installer using `Build-ZERO-C.ps1`.
- [ ] Smoke-test compiling and linking from the default install path containing
      spaces; verify the bundled default manifest resolves.
- [ ] Smoke-test setup, launch, opening/saving a disposable file, building,
      diagnostics, Run, shortcuts, and uninstall on a clean Windows account.
- [ ] Verify first launch creates `Documents\ZERO C` and that the Explorer
      and New/Open/Save dialogs use it as the default workspace.
- [ ] Verify files opened outside the workspace still save at their original
      paths and that the app does not move or package personal source files.
- [ ] Extract the portable ZIP to a writable folder and USB drive; verify
      launch, GCC build/run, settings persistence, and no external dependencies.
- [ ] Verify portable `C\TC` includes only the compatibility header before
      packaging; do not bundle any personal source programs.
- [ ] Confirm the final installer contains no personal source files.
- [ ] Confirm the repository's `C\TC` tree contains only `compat\conio.h`.
- [ ] Record SHA-256 of the installer and toolchain release/version.
- [ ] Publish the installer as a GitHub Release asset and provide source and
      licensing information for bundled third-party components.
- [ ] If possible, code-sign the installer with a trusted publisher identity.
- [ ] Update the version consistently in `ZERO-C-Setup.iss`, version resources,
      documentation, and release notes before making a new release.

Do not describe an unsigned build as officially verified or signed. Avoid
including personal practice programs or existing compiled executables in
source-control releases.
