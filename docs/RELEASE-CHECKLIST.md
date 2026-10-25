# ZERO C release checklist

- [ ] Review the application source and third-party notices.
- [ ] Use a clean, supported x64 WinLibs/MinGW-w64 GCC toolchain.
- [ ] Install Inno Setup 7 and verify its official installer signature.
- [ ] Build the app and installer using `Build-ZERO-C.ps1`.
- [ ] Smoke-test setup, launch, opening/saving a disposable file, building,
      diagnostics, Run, shortcuts, and uninstall on a clean Windows account.
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
