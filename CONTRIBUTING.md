# Contributing to QFTbx

Contributions are welcome: a bug report with the project file that shows
it, a fix, a new algorithm from the literature, a better explanation in the
documentation.

## Building and testing

The requirements, the options and the compilers are in
[docs/BUILDING.md](docs/BUILDING.md); the short version:

    cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
    cmake --build build -j
    ctest --test-dir build --output-on-failure

The tests are described in [docs/TESTING.md](docs/TESTING.md). Every
behavioural change needs a test, and a bug fix needs a test that fails
before the fix. A change that alters a golden result says so in its commit
and re-pins the golden deliberately.

## Branches

`main` holds releases. Day-to-day work is merged into `Development`. Branch
from `Development`, keep each commit to one topic, and open a pull request
back to `Development`; the continuous integration (`.github/workflows/ci.yml`)
builds the project with Qt 6.5 on Linux and runs the suite, and it must be
green.

## Conventions

- **English everywhere**: identifiers, comments, commit messages,
  documentation.
- **C++20, value semantics first.** No raw `new` and `delete`; standard
  containers, `std::unique_ptr` where ownership is needed. The one
  exception is Qt's parent-child ownership in the GUI, which is Qt's own
  memory management and must not be doubled with a smart pointer.
- **Qt only in the GUI.** The core (`src/core/`) and the persistence
  (`src/persistence/`) use the standard library; the target
  `qftbx_core_noqt` compiles them without Qt on the include path to prove
  it.
- **Errors are exceptions.** The core never talks to the user: it throws
  `qftbx::Exception` or a subclass (`src/core/common/exception.h`) and the
  GUI catches at its boundary and shows the message. A slot that reaches
  the core catches, because an exception escaping a Qt slot terminates the
  process.
- **Formatting**: `.clang-format` at the root; `.clang-tidy` is advisory.
- **Doxygen**: algorithm classes get full headers, with the mathematics and
  the reference to the paper; the public API gets a `\brief`; trivial
  internals get nothing. Never restate the signature in words.
- **Commit messages** explain the change and the reason in prose, in
  English, so that the history reads as the record of the decisions.

## Documentation

Each kind of documentation has its place: the build in `docs/BUILDING.md`,
the modules in `docs/ARCHITECTURE.md`, the algorithms in `docs/algorithms/`
(one page per algorithm, with the paper it follows and where it departs from
it), the settings in `docs/CONFIGURATION.md`, the file format in
`docs/PROJECT_FORMAT.md`, the tests in `docs/TESTING.md`. A change that
moves a file, adds an option or alters an algorithm updates the page that
describes it in the same pull request.
