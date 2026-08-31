# Contributing to QFTbx

Thanks for your interest in QFTbx! Contributions are welcome.

> **Heads-up:** the codebase is undergoing a deep modernisation (see
> [REFACTOR_PLAN.md](REFACTOR_PLAN.md)). Parts of the code still use legacy
> conventions (Spanish identifiers, raw pointers, Qt containers in the
> backend); new code must follow the conventions below, and legacy code is
> being migrated module by module.

## Building

Requirements: CMake ≥ 3.17, Qt ≥ 6.x, a C++20 compiler. Optional: OpenMP, CUDA, Doxygen.

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
./build/QFTbx
```

If Qt is not found automatically, pass `-DCMAKE_PREFIX_PATH=/path/to/Qt/6.x/gcc_64`.

## Tests

Unit tests use GoogleTest (fetched automatically) and run through CTest:

```bash
ctest --test-dir build --output-on-failure
```

Tests build by default (`QFTBX_BUILD_TESTS=ON`). Every behavioural change needs a test;
bug fixes need a test that fails before the fix. For memory checking, configure with
`-DQFTBX_SANITIZERS=address,undefined` (while the refactor lasts, run the tests with
`ASAN_OPTIONS=detect_leaks=0`).

## Branches and pull requests

- `main` holds releases; day-to-day work is merged into `Development`.
- Branch from `Development`, keep commits atomic (one topic per commit), and open a
  pull request back to `Development`. CI must be green.

## Code conventions

- **Language**: everything in English — identifiers, comments, commit messages, docs.
- **C++20, value semantics first**: no raw `new`/`delete`; use standard containers and
  `std::unique_ptr` where ownership is needed.
- **Qt only in the GUI**: the backend (`Modelo/`, `DAO/`, `src/persistence/`) uses the standard
  library. Exception: the XML persistence module keeps `QXmlStream` internally.
- **Errors**: the backend never talks to the user; it throws `qftbx::Exception` (see
  `Modelo/Herramientas/exception.h`) and GUI slots catch and display.
- **Formatting**: `.clang-format` at the repo root; `.clang-tidy` is advisory.
- **Doxygen**: algorithm classes get full headers (math, paper references); public API
  gets a one-line `\brief`; trivial internals get nothing. Never restate the signature.

## Architecture

See [docs/ARCHITECTURE.md](docs/ARCHITECTURE.md) for a one-page tour of the modules
and the QFT design pipeline.
