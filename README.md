# Bapad

A lightweight text editor for Windows — built as a learning project following
the [NeatPad tutorial](https://www.catch22.net/tuts/neatpad/neatpad-overview) by
catch22.net, with cross-platform Qt6 port.

## Variants

| Variant | Platform | Build |
|---------|----------|-------|
| **Bapad** | Win32 native | Visual Studio 2022 (`Bapad/Bapad.sln`) |
| **BapadQt** | Qt6 cross-platform | CMake (`BapadQt/CMakeLists.txt`) |

Both share the same text engine library (`Bapad/TextDocument/`).

## Architecture

```
                         ┌──────────────────────────┐
                         │   Bapad (Win32)           │
  TextView (UI layer)    │   - Message loop          │
                         │   - Keyboard/mouse/paint  │
                         │   - File I/O              │
                         └──────────┬───────────────┘
                                     │
                         ┌──────────┴───────────────┐
                         │   BapadQt (Qt6)           │
                         │   - QAbstractScrollArea   │
                         │   - QPainter rendering    │
                         │   - Clipboard / IME       │
                         └──────────┬───────────────┘
                                     │
                         ┌──────────┴───────────────┐
  TextDocument           │   TextDocument            │
  (high-level API)       │   - Insert/Erase/Replace  │
                         │   - Undo / Redo           │
                         │   - Line iteration        │
                         └──────────┬───────────────┘
                                     │
                  ┌──────────────────┼──────────────────┐
                  │                  │                  │
         ┌────────┴───────┐ ┌───────┴────────┐ ┌──────┴───────┐
  Core   │  PieceTree     │ │ FormatConvV2   │ │ (helpers)    │
  lib    │  - Piece Table │ │ - BOM detect   │ │ createLine-  │
         │  - Insert/     │ │ - UTF8/16/32   │ │ Starts       │
         │    Erase/      │ │   conversion   │ │ Normalize-   │
         │    Replace     │ │ - IsUTF8       │ │ LineEndings  │
         │  - GetLine/    │ │ - Normalize    │ │              │
         │    GetText     │ │   LineEndings  │ │              │
         └────────────────┘ └────────────────┘ └──────────────┘
```

### Layers

**TextDocument** — high-level document API. Coordinates edits, manages undo/redo
stacks, and provides line- and offset-based iteration.

**PieceTree** — a [piece table](https://en.wikipedia.org/wiki/Piece_table)
data structure for efficient text editing. Stores the original content in one
buffer (index 1) and inserted text in a separate change buffer (index 0).
Pieces reference ranges within these buffers; edits only append to the change
buffer without moving existing data.

**FormatConversionV2** — file format detection and text encoding conversion.
Detects byte-order marks (UTF-8, UTF-16 LE/BE, UTF-32 LE/BE), validates UTF-8
content, converts raw bytes to UTF-16 via ICU, and normalizes line endings
(CR, CRLF → LF).

## Dependencies

### Bapad (Win32)

| Dependency | Source | Purpose |
|-----------|--------|---------|
| [Microsoft.GSL](https://github.com/microsoft/GSL) | vcpkg (`ms-gsl`) | `gsl::span`, `gsl::at` |
| [ICU](https://icu.unicode.org/) | vcpkg (`icu`) | UTF-8/16 encoding conversion |
| [Google Test](https://github.com/google/googletest) | NuGet | Unit testing |

vcpkg dependencies are declared in `Bapad/vcpkg.json`. The solution opens
via `Bapad/Bapad.sln`.

### BapadQt (Qt6)

| Dependency | Source | Purpose |
|-----------|--------|---------|
| Qt 6.5+ | system / vcpkg | Core, Widgets |
| Microsoft.GSL | vcpkg | Same as above |
| ICU | vcpkg | Same as above |

## Building

### Bapad (Win32)

1. Install [vcpkg](https://vcpkg.io/) and set `VCPKG_ROOT` environment variable.
2. Install dependencies:
   ```
   vcpkg install ms-gsl icu --triplet x64-windows
   ```
3. Open `Bapad/Bapad.sln` in Visual Studio 2022.
4. Restore NuGet packages (Google Test).
5. Build **x64 | Debug** or **x64 | Release**.

### BapadQt

```
cd BapadQt
cmake -B build -DCMAKE_TOOLCHAIN_FILE="%VCPKG_ROOT%/scripts/buildsystems/vcpkg.cmake"
cmake --build build
```

Requires Qt 6.5+ accessible via CMake's `find_package`.

## Testing

Unit tests use Google Test (gtest 1.8.1 via NuGet).

**Run in VS:** Test → Test Explorer → Run All.

**Test files** (in `Bapad/UnitTests/`):

| File | Coverage |
|------|----------|
| `PieceTree_Tests.cpp` | PieceTree constructor, Insert/Erase/Replace, GetText, GetLine |
| `TextDocument_Tests.cpp` | TextDocument Initialize/Clear, edit ops, Undo/Redo, line iteration |
| `FormatConversion_Tests.cpp` | DetectFileFormat, IsUTF8, RawToUtf16, SwapWord16 |
| `Utility_Tests.cpp` | NormalizeLineEndings, GetLineIndexFromNodePosistion, createLineStarts |

**Coverage report** (requires [OpenCppCoverage](https://github.com/OpenCppCoverage/OpenCppCoverage)):

```
OpenCppCoverage.exe --sources "Bapad\TextDocument" -- .\x64\Debug\UnitTests.exe
```

## Inspiration

- [NeatPad tutorial](https://www.catch22.net/tuts/neatpad/neatpad-overview) by
  catch22.net — the original tutorial this project follows
- [Piece table](https://en.wikipedia.org/wiki/Piece_table) — core data structure
