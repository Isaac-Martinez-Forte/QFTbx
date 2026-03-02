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

---

## Configuration options

Dependencies and features can be enabled or disabled directly from the main CMakeLists.txt file using the following options:

OPTION (USE_CLANG    "Use CLANG"    OFF)
OPTION (USE_OpenMP   "Use OpenMP"   ON)
OPTION (USE_CUDA     "Use CUDA"     OFF)
OPTION (USE_Doxygen  "Use Doxygen"  OFF)

Automatic configuration is applied based on the selected options and the available system libraries.

---

## Build instructions

Linux / Windows (MinGW)

mkdir build
cd build
cmake ..
make

If all dependencies are correctly installed, the project can be compiled on Linux (using GCC or Clang) or Windows (using MinGW).

---

## Execution

From the build directory:

./QFTbx

No installer is currently provided. The application is intended to be run directly from the build directory.

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
