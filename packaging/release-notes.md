A beta of QFTbx, the toolbox for robust controller design with Quantitative
Feedback Theory. It is under active development: results may change between
versions, and some parts are experimental.

## Downloads

| System | File |
|---|---|
| Linux, x86-64, with glibc 2.35 or later (Ubuntu 22.04, Debian 12, Fedora 36 or later) | `QFTbx-…-linux-x86_64.AppImage` |
| Windows 10 or 11, 64-bit | `QFTbx-…-setup.exe` installs for the user, or for every user when asked; `QFTbx-….msi` installs for the whole machine; `QFTbx-…-portable.zip` runs from any folder |
| macOS 12 or later, Apple silicon | `QFTbx-…-macos-arm64.dmg` |

`SHA256SUMS` holds the checksum of every file.

## Opening it the first time

The packages are not signed yet, so each system asks once:

- **Linux**: make the AppImage executable, `chmod +x QFTbx-*.AppImage`, and run it.
- **Windows**: if SmartScreen says it protected the computer, choose *More info*, then *Run anyway*.
- **macOS**: open the application once, then choose *Open Anyway* in System Settings, Privacy & Security.

The examples come with every package, under *Examples* in the menu bar. The
packages are built to run on any machine of their kind; building from the
sources, as the README describes, gives the fastest program for the machine
that compiles it.
