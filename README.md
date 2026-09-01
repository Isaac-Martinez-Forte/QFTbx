<p align="center">
  <img src="Resources/Icon/QFTbx_256.png" width="96"/>
</p>

<p align="center">
  <img src="https://img.shields.io/badge/C%2B%2B-20-blue"/>
  <img src="https://img.shields.io/badge/Qt-6-green"/>
</p>

<h1 align="center">QFTbx</h1>

<p align="center">
Quantitative Feedback Theory toolbox for robust control design<br/>
Academic-oriented software for QFT-based analysis and automatic loop shaping
</p>

---

## Overview

QFTbx is a graphical and computational toolbox for the design and analysis of robust controllers using Quantitative Feedback Theory (QFT).

The software is mainly oriented towards academic and research use, although it can also be useful for control engineers interested in QFT-based methodologies.
It provides tools for automatic loop shaping, interval analysis, and visualization of QFT constraints.

This software is currently under active development. Some features may be incomplete or experimental.

---

## Dependencies

Required
- CMake >= 3.17
- Qt >= 6.x
- C++ compiler with C++20 support

Optional
- OpenMP >= 11
- CUDA >= 7
- Doxygen (for documentation generation)

Bundled or fetched automatically (nothing to install by hand)
- C-XSC, the interval arithmetic library: vendored in `3rd-party/cxsc` and
  built as an isolated ExternalProject. It is the numerical foundation of
  the loop-shaping algorithms.
- muParserX (expression evaluation) and QCustomPlot (plots): vendored in
  `3rd-party/`.
- pugixml (project files): fetched with FetchContent at configure time,
  so the first configuration needs network access.

---

## Configuration options

Dependencies and features can be enabled or disabled directly from the main CMakeLists.txt file using the following options:

OPTION (USE_CLANG       "Use CLANG"             OFF)
OPTION (USE_OpenMP      "Use OpenMP"            ON)
OPTION (USE_CUDA        "Use CUDA"              OFF)
OPTION (USE_Doxygen     "Use Doxygen"           OFF)
OPTION (USE_NATIVE_ARCH "Enable -march=native"  ON)
OPTION (QFTBX_BUILD_TESTS "Build unit tests"    ON)

Automatic configuration is applied based on the selected options and the available system libraries.

### A note on -frounding-math

The backend and the test binary are compiled with `-frounding-math`, and
this is not optional. C-XSC implements its directed rounding with inline
assembly in its headers; at -O3 the inliner reorders it and the interval
arithmetic silently stops being rigorous, which would invalidate the
global-optimality guarantee of the loop-shaping algorithms. The flag is
applied per target (the application itself does not use it: it breaks
Qt's constexpr float code), and `tests/backend/cxsc_rigor_test.cpp`
fails deterministically if a build change ever breaks the rounding
again.

---

## Build instructions

Linux / Windows (MinGW)

mkdir build
cd build
cmake ..
make

If all dependencies are correctly installed, the project can be compiled on Linux (using GCC or Clang) or Windows (using MinGW).

---

## Tests

The project ships a suite of unit, characterisation and golden tests
(GoogleTest, fetched at configure time). It is built by default; run it
from the build directory:

    ctest

or, for the detail of a failure:

    ctest --output-on-failure -R <test name>

The suite covers the plant/uncertainty model, the frequency set, the
specifications, the templates and their contours, the boundaries, the
persistence round-trip, the five loop-shaping algorithms (against
published results where they exist) and the rigour of the interval
arithmetic. The goldens pin current behaviour; correctness of each
algorithm is judged against its paper.

---

## Execution

From the build directory:

./QFTbx

No installer is currently provided. The application is intended to be run directly from the build directory.

---

## Source layout

    src/core/          computational core, free of GUI code
      system/            plants and controllers (LtiSystem and its forms)
      frequencies/       the design frequency set
      specifications/    design specifications
      templates/         template computation and contours
      boundaries/        boundary computation, tracing and union
      loopshaping/       the five algorithms (NT, NK, MR, MC1, MC thesis)
      gpu/               CUDA kernels (optional)
      math/              numeric sequences
      project_controller the application facade (owns the project data)
    src/persistence/   .qft project reading and writing
    GUI/               Qt dialogs, viewers and the main window
    tests/backend/     the computational test suite
    tests/gui/         the headless dialog smoke suite
    3rd-party/         vendored dependencies

---

## Documentation

Full documentation is not currently bundled.
However, Doxygen can be used to generate partial documentation if enabled during configuration.

---

## License

This project is distributed under the GNU General Public License, Version 3 (GPLv3).

---

## Authors

Isaac Martínez Forte
isaac.martinez@upct.es

Joaquín Cervera López
jcervera@um.es

---

## Bibliography

Martínez-Forte, I., & Cervera, J. (2021).
Accelerated quantitative feedback theory interval automatic loop shaping algorithm.
International Journal of Robust and Nonlinear Control, 31(9), 4378–4396.
https://doi.org/10.1002/rnc.5499
http://hdl.handle.net/10201/123363

Martínez-Forte, I., & Cervera, J. (2022).
Aceleración de algoritmos intervalares de ajuste automático del lazo en QFT.
Doctoral Thesis, Universidad de Murcia.
http://hdl.handle.net/10201/122610

Martínez-Forte, I. (2013).
QFTbx, herramienta de diseño QFT: especificación de requisitos y prototipado.
Final Degree Project, Universidad de Murcia.
http://hdl.handle.net/10201/61459

Martínez-Forte, I. (2014).
Paralelización de algoritmos QFT mediante OpenMP y CUDA.
Master's Thesis, Universidad de Murcia.
http://hdl.handle.net/10201/61460
