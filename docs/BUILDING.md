# Building QFTbx

QFTbx is a CMake project: one configuration step, one build step, and the
application and the tests come out of the same tree.

## Requirements

**A C++20 compiler.** GCC 8 or later (the toolbox is developed with GCC 8.5
and built by the continuous integration with the current Ubuntu's), Clang 11
or later, or MinGW-w64 on Windows. The code uses no
extension of any of them. Clang uses the standard library of the GCC it
finds, and Qt 6.5 needs a C++20 one, GCC 10's or later: on a machine whose
system GCC is older, Clang needs `--gcc-toolchain` pointed at a newer one.

**CMake 3.17 or later.**

**Qt 6.** The application links the Core, Widgets and PrintSupport modules,
and the build uses LinguistTools for the Spanish translation. Qt 6.5 is what
the continuous integration builds against. The computational core and the
persistence link no Qt at all, and a build target checks that they do not
(`qftbx_core_noqt`, see below).

**Optional.** OpenMP, for the parallel template sweep and boundary
computation; it comes with the compiler and is used when found. CUDA 11 or
later for the GPU kernels, off by default. Doxygen, with Graphviz for the
diagrams, for the API reference.

**Fetched at configure time.** pugixml 1.14 (the `.qft` reader and writer)
and GoogleTest 1.14 (the tests) are downloaded with FetchContent the first
time a build tree is configured, so that configuration needs network access.
C-XSC is fetched the same way when it is chosen as the interval backend.
Everything else is vendored under `3rd-party/`.

## Configure and build

    cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
    cmake --build build -j
    ./build/QFTbx

`CMAKE_BUILD_TYPE` matters: the loop-shaping algorithms are several times
slower in a Debug build, and every timing in the documentation is a Release
one. Use Debug for stepping through the code and Release for using or
measuring the toolbox.

**Qt not found.** CMake looks for Qt in the usual places; when it is
installed elsewhere, name the installation:

    cmake -S . -B build -DCMAKE_PREFIX_PATH=/opt/Qt/6.5.1/gcc_64

On Windows the build defaults to `C:/Qt/6.6.2/mingw_64` when nothing is
given, and `-DCMAKE_PREFIX_PATH` overrides it as above.

**Windows.** Build with MinGW-w64 and the MinGW build of Qt:

    cmake -S . -B build -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH=C:/Qt/6.6.2/mingw_64
    cmake --build build -j
    build\QFTbx.exe

The Qt DLLs have to be on the path or next to the executable (`windeployqt`
copies them).

**Running.** The application runs from the build directory; there is no
installer yet. On Linux the build writes a `.desktop` entry next to the
executable, so the application can be added to a launcher. Settings are read
from an optional `qftbx.conf`, described in [CONFIGURATION.md](CONFIGURATION.md).

## Options

Every option is passed to CMake as `-D<option>=<value>` and is remembered by
the build tree.

| Option | Default | Effect |
|---|---|---|
| `CMAKE_BUILD_TYPE` | none | `Release` for use and measurement, `Debug` for development |
| `USE_OpenMP` | `ON` | Parallel templates and boundaries when the compiler supports OpenMP |
| `USE_NATIVE_ARCH` | `ON` | `-march=native`: the numeric code tuned for the machine that builds it. Turn off for a binary that must run on other machines, and for tools that cannot run the wide vector instructions (Valgrind). With AVX-512 the compiler is also asked to keep its vectors at 256 bits, which measured faster here |
| `USE_CLANG` | `OFF` | Compile with Clang when it is installed. The compiler is chosen before the project is configured and cannot change afterwards, so this takes effect in a fresh build directory; an explicit `-DCMAKE_CXX_COMPILER` always wins |
| `USE_CUDA` | `OFF` | The CUDA kernels of the templates and boundaries, when a CUDA compiler is found. `CMAKE_CUDA_ARCHITECTURES` names the cards (default 61, 75 and 86) |
| `QFTBX_BUILD_TESTS` | `ON` | Build the two test binaries and register them with CTest |
| `QFTBX_BUILD_BENCHMARK` | `OFF` | Build the benchmark tool `qftbx-bench`, its tests and the benchmark planner of the interface. For measuring the algorithms, not for using them; see [BENCHMARKING.md](BENCHMARKING.md) |
| `QFTBX_INTERVAL_BACKEND` | `kv` | The interval arithmetic library: `kv` or `cxsc`. See [INTERVAL_ARITHMETIC.md](INTERVAL_ARITHMETIC.md) |
| `QFTBX_SANITIZERS` | empty | Comma-separated sanitizers for a development build, e.g. `address,undefined`; needs the corresponding runtimes |
| `FETCHCONTENT_SOURCE_DIR_CXSC` | none | A local checkout of the C-XSC fork, for a `cxsc` build without network access |

Other compilers are chosen the CMake way, `-DCMAKE_CXX_COMPILER=<path>`, in
a fresh build directory.

## Targets

| Target | What it is |
|---|---|
| `QFTbx` | The application |
| `qftbx_core` | The computational core: plants, frequencies, specifications, templates, boundaries, loop shaping. No Qt |
| `qftbx_persistence` | The `.qft` reader and writer. No Qt |
| `qftbx_app` | The facade between the GUI and the core (`ProjectController`) |
| `qftbx_gui` | The Qt dialogs, viewers and main window |
| `qftbx_tests`, `qftbx_gui_tests` | The backend suite and the headless GUI smoke suite |
| `qftbx_core_noqt` | Not built by default: compiles the core and the persistence with no Qt on the include path, to prove they need none. `cmake --build build --target qftbx_core_noqt` |
| `docs` | The Doxygen reference; only exists when Doxygen is installed |
| `qftbx-bench`, `qftbx_bench`, `qftbx_bench_tests` | The benchmark tool, its library and its tests; only with `QFTBX_BUILD_BENCHMARK` |

Each folder of the source tree has a `CMakeLists.txt` that adds its own files
to the target it belongs to (the helpers are in `cmake/QftbxFunctions.cmake`),
so no file list is kept by hand: a new source file is picked up at the next
configuration.

## Translations

The interface is written in English and translated into Spanish; the user
picks the language under View, and the choice is written to the settings
file (`interface.language`). The sources of the translations are the `.ts`
files under `src/gui/translations/`, one per language, compiled by
`lrelease` at build time into `.qm` files that go into the application's
resources, so nothing has to be installed next to the executable; the View
menu lists whatever translations were compiled in. How to add a language is
in [src/gui/translations/README.md](../src/gui/translations/README.md). A build
never rewrites the `.ts`: after adding or changing a `tr()` string or a text
in a `.ui` file, run

    cmake --build build --target update_translations

which runs `lupdate` over the GUI sources and over the core, the persistence
and the facade, whose user-facing messages are written inside
`QFTBX_TR("Core", "...")` (see `src/core/common/message.h`), and adds the new
strings to the file marked as unfinished, then give them their Spanish text
(Qt Linguist or a text editor) and rebuild. A test of the GUI suite fails while a string is
left untranslated or an obsolete one is left in the file. Run the target from
a tree configured with `QFTBX_BUILD_BENCHMARK` so the planner's strings are
included too.

## The API reference

    cmake --build build --target docs

or, from the root of the repository, plainly `doxygen`. Both read the same
`Doxyfile`, so there is no generated copy of the configuration that can
drift from the one under version control. The result lands in
`docs/api/html/index.html`, which is not committed; its landing page is
[ARCHITECTURE.md](ARCHITECTURE.md). Without Graphviz the pages have no
inheritance or include diagrams, and CMake says so at configure time.

Two things worth knowing about the configuration:

- Formulas are rendered by MathJax from a CDN, because there is no LaTeX in
  the loop. The pages need internet access for the formulas and for nothing
  else; a local copy of MathJax and `MATHJAX_RELPATH` make them
  self-contained.
- Warnings fail the run (`WARN_AS_ERROR`), but only the ones that are
  documentation bugs: a parameter that does not exist, a half-documented
  signature, a broken reference. Undocumented trivial accessors are not
  warned about, by policy: the documentation explains the algorithms and the
  API and leaves trivia alone. `STRIP_CODE_COMMENTS` is off on purpose,
  because much of the reasoning lives in ordinary comments beside the code
  and the generated source browser is where to read it.

## Sanitizers

    cmake -S . -B build-asan -DCMAKE_BUILD_TYPE=Debug -DQFTBX_SANITIZERS=address,undefined
    cmake --build build-asan -j
    ASAN_OPTIONS=detect_leaks=0 ctest --test-dir build-asan

Leak detection is switched off because Qt's global objects report leaks that
are not ours. Valgrind needs `-DUSE_NATIVE_ARCH=OFF`, since it cannot
execute the AVX-512 instructions a native build may contain.
