<p align="center">
  <img src="docs/qftbx_banner.svg" width="840" alt="QFTbx"/>
</p>

<p align="center">
  <img src="https://img.shields.io/badge/C%2B%2B-20-blue"/>
  <img src="https://img.shields.io/badge/Qt-6-green"/>
  <img src="https://img.shields.io/badge/license-GPLv3-lightgrey"/>
</p>

<p align="center">
Robust controller design with Quantitative Feedback Theory: templates,
boundaries and certified automatic loop shaping
</p>

---

## Overview

QFTbx is a desktop toolbox for the design and analysis of robust controllers
with Quantitative Feedback Theory (QFT). It walks a design through the
standard QFT pipeline, from the uncertain plant to the shaped loop, and
computes every step:

- **Templates**: the value sets of an uncertain plant at each design
  frequency, reduced to their contour with the ε-hull algorithm.
- **Boundaries**: the regions of the Nichols chart that the nominal loop must
  respect for each specification (stability, tracking, disturbance rejection,
  control effort), merged into one boundary per frequency.
- **Automatic loop shaping**: five interval branch and bound algorithms that
  find a controller of a given structure with the least high-frequency gain,
  and certify it (NT, NK, MR and the two accelerated MC algorithms of the
  author's doctoral work).
- **Rigorous arithmetic**: the loop shaping runs on verified interval
  arithmetic, so a controller reported feasible is feasible for every plant
  of the uncertainty set.

The software comes out of academic work at the University of Murcia and is
oriented to research and teaching; it is also usable by control engineers
working with QFT. It is under active development, and some parts are
experimental.

---

## Requirements

| Dependency | Version | Needed for | How it is obtained |
|---|---|---|---|
| C++ compiler | C++20: GCC 8 or later, Clang 11 or later, MinGW-w64 | everything | installed by hand |
| CMake | 3.17 or later | the build | installed by hand |
| Qt | 6.x (Core, Widgets, PrintSupport, LinguistTools) | the application and the GUI tests | installed by hand |
| OpenMP | any supported by the compiler | parallel templates and boundaries, optional | comes with the compiler |
| CUDA | 11 or later | GPU kernels, optional, off by default | installed by hand |
| Doxygen and Graphviz | recent | the API documentation, optional | installed by hand |
| kv | 0.4.62 | interval arithmetic (default backend) | vendored in `3rd-party/kv` |
| C-XSC | 2.5.4, [QFTbx fork](https://github.com/Isaac-Martinez-Forte/cxsc-cpp17) | interval arithmetic (alternative backend), optional | fetched at configure time |
| QCustomPlot | 2.x | the plots | vendored in `3rd-party/qcustomplot` |
| pugixml | 1.14 | `.qft` project files | fetched at configure time |
| GoogleTest | 1.14 | the tests | fetched at configure time |

The first configuration downloads pugixml and GoogleTest, so it needs network
access; after that the build is self-contained.

---

## Quick start

Linux:

    cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
    cmake --build build -j
    ./build/QFTbx

Windows (MinGW-w64, from a shell where the compiler is on the path):

    cmake -S . -B build -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH=C:/Qt/6.6.2/mingw_64
    cmake --build build -j
    build\QFTbx.exe

If Qt is not found, pass `-DCMAKE_PREFIX_PATH=<Qt installation>`. Every
option, the compilers, the interval backends and the documentation target are
described in [docs/BUILDING.md](docs/BUILDING.md).

The tests are built with the project and run with `ctest --test-dir build`.

No installer is provided yet; the application runs from the build directory.

---

## Documentation

| Document | Contents |
|---|---|
| [docs/BUILDING.md](docs/BUILDING.md) | Requirements in detail, every CMake option, compilers, Windows, sanitizers, the Doxygen reference |
| [docs/ARCHITECTURE.md](docs/ARCHITECTURE.md) | The QFT pipeline, the modules and their responsibilities, error handling, persistence |
| [docs/algorithms/](docs/algorithms/README.md) | Every algorithm: what it solves, the paper it comes from, how the implementation follows it, the files and the tests |
| [docs/INTERVAL_ARITHMETIC.md](docs/INTERVAL_ARITHMETIC.md) | The interval layer, the kv and C-XSC backends, how they are cross-checked |
| [docs/CONFIGURATION.md](docs/CONFIGURATION.md) | The `qftbx.conf` settings file: where it is read from, every key and its range |
| [docs/PROJECT_FORMAT.md](docs/PROJECT_FORMAT.md) | The `.qft` project file |
| [docs/TESTING.md](docs/TESTING.md) | Running and reading the test suites, the fixtures, the golden policy |
| [CONTRIBUTING.md](CONTRIBUTING.md) | Branches, conventions and how a change gets in |

The API reference of the classes is generated with Doxygen (`cmake --build
build --target docs`, or `doxygen` from the root) into `docs/api/html`.

---

## License

QFTbx is distributed under the GNU General Public License, version 3
([LICENSE](LICENSE)). The vendored and fetched libraries keep their own
licences: kv (MIT), QCustomPlot (GPLv3), pugixml (MIT), GoogleTest (BSD
3-clause), C-XSC (LGPL 2.1).

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

Martínez-Forte, I. (2022).
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
