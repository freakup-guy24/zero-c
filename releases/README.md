GitHub Release installer downloads

Do not commit the installer binary here. GitHub limits individual Git files to 100 MB, while the complete GCC-bundled installer is larger. Upload the installer and its .sha256 checksum as GitHub Release assets, or create a v1.0.0 tag after pushing the source to trigger the tagged-release workflow.

The local build process outputs the installer and checksum to `dist/`. The GitHub Actions tag workflow builds those files and attaches them to a release.
