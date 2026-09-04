# References

Every algorithm of QFTbx comes from one of the documents below. The author's own
works are included in this folder as PDF files. The third-party works are cited
with their DOI or original location and are not redistributed: their copyright
belongs to their publishers or their authors, and most of them are available
through a university library.

## The author's documents (included)

**[martinez-forte-2013-pfc-qftbx.pdf](martinez-forte-2013-pfc-qftbx.pdf)**
I. Martínez Forte, *QFTbx, herramienta de diseño QFT: especificación de
requisitos y prototipado*. Proyecto Fin de Carrera (degree project), Ingeniería
Informática, Facultad de Informática, Universidad de Murcia, September 2013.
Directed by J. Cervera López. In Spanish.
The origin of the toolbox: the requirements, the prototype, and appendix B with
the template, ε-hull and boundary algorithms. Used by
[templates.md](../templates.md) and [boundaries.md](../boundaries.md).

**[martinez-forte-2014-tfm-openmp-cuda.pdf](martinez-forte-2014-tfm-openmp-cuda.pdf)**
I. Martínez Forte, *Paralelización de algoritmos QFT mediante OpenMP y CUDA*.
Trabajo Fin de Máster (master's thesis), Máster en Nuevas Tecnologías de la
Informática, Universidad de Murcia, July 2014. Directed by J. Cervera López. In
Spanish.
The parallel CPU (OpenMP) and GPU (CUDA) versions of the template and boundary
algorithms, with their experimental study. Used by [templates.md](../templates.md)
and [boundaries.md](../boundaries.md).

**[martinez-forte-2022-phd-thesis.pdf](martinez-forte-2022-phd-thesis.pdf)**
I. Martínez-Forte, *Aceleración de algoritmos intervalares de ajuste automático
del lazo en QFT*. Doctoral thesis, Universidad de Murcia, March 2022. Directed by
J. Cervera López. In Spanish. Open access in the university repository DIGITUM,
handle 10201/122610 (http://hdl.handle.net/10201/122610).
Chapters 1 and 3 present interval arithmetic, interval global search and the NT
and NK algorithms; chapters 4 and 5 the strategies and the MC algorithm; chapter 6
the benchmarks; chapter 7 the software. Used by every loop-shaping page.

## The author's paper (open access, not included here)

I. Martínez-Forte and J. Cervera, *Accelerated quantitative feedback theory
interval automatic loop shaping algorithm*. International Journal of Robust and
Nonlinear Control 31:4378–4396, 2021. DOI 10.1002/rnc.5499.
Copyright 2021 John Wiley & Sons Ltd. The published version is Wiley's. The
accepted manuscript, the version before Wiley's typesetting, is in open access in
the university repository DIGITUM, handle 10201/123363
(http://hdl.handle.net/10201/123363), under a CC BY-NC-ND licence, as Wiley's
self-archiving terms allow. Read it there. The algorithm is the one of
[loop-shaping-mc1.md](../loop-shaping-mc1.md), and the thesis above covers the
same material in chapters 4 and 5.

## Third-party references (not included)

**Loop shaping**

- S. Tharewal, *Automated Synthesis of QFT Controllers and Prefilters using
  Interval Global Optimization Techniques*. PhD thesis, IDP in Systems and
  Control Engineering, Indian Institute of Technology Bombay, 2005. Supervised by
  P. S. V. Nataraj. Algorithm NT; the section numbers cited by the
  implementation are its. The department distributed it for years at
  `sc.iitb.ac.in/theses/phd_theses/sachin_thesis.zip`, a location that no longer
  answers; the copyright stays with the author and the institute.
- S. V. Paluri (P. S. V. Nataraj) and N. Kubal, *Automatic loop shaping in QFT
  using hybrid optimization and constraint propagation techniques*. International
  Journal of Robust and Nonlinear Control 17:251–264, 2007.
  DOI 10.1002/rnc.1085. Copyright 2006 John Wiley & Sons, Ltd. Algorithm NK and
  the Quick Solution equations.
- R. Kalla and P. S. V. Nataraj, *Synthesis of fractional-order QFT controllers
  using interval constraint satisfaction technique*. Proceedings of the 4th IFAC
  Workshop on Fractional Differentiation and its Applications (FDA'10), Badajoz,
  Spain, 2010. Algorithm MR.
- P. S. V. Nataraj and M. Deshpande, *Automated synthesis of fixed structure QFT
  controller using interval constraint satisfaction techniques*. 17th IFAC World
  Congress, Seoul, 2008. Copyright 2008 IFAC. The first ICSP formulation of the
  problem; background for MR.
- N. Cohen, Y. Chait, O. Yaniv and C. Borghesani, *Stability analysis using
  Nichols charts*. International Journal of Robust and Nonlinear Control
  4(1):3–20, 1994. The nominal stability criterion, the one the MATLAB QFT
  Toolbox applies.
- R. B. Kearfott, *Rigorous Global Search: Continuous Problems*. Kluwer, 1996.
  Background on interval branch & bound.

**Templates**

- M. Nordin, *Uncertain systems with backlash: modeling, identification and
  synthesis*. Licentiate thesis, Royal Institute of Technology (KTH), Stockholm,
  1993. Defines the ε-hull that gives a template its contour; QFTbx knows it
  through the two works below, which cite it.
- F. J. Montoya Dato, *Diseño de sistemas de control no lineales mediante QFT:
  análisis computacional y desarrollo de una herramienta CACSD*. Doctoral thesis,
  Universidad de Murcia, 1998. Directed by A. Baños Torrico. The MATLAB
  implementation of the ε-hull (`EPSHULL.M`, function `epsh2`) belongs to the
  CACSD tool of this thesis; the QFTbx implementation is a faithful port of it.
- J. Cervera, A. Baños and I. M. Horowitz, *Computation of SISO general plant
  templates*. 5th International Symposium on QFT and Robust Frequency Domain
  Methods, Pamplona, 2001. Computes the template of a plant built from several
  uncertain single plants, and takes the ε-hull of Nordin in Montoya's
  implementation for the contours; the path by which the algorithm reached QFTbx.

**Boundaries**

- J. C. Moreno, A. Baños and M. Berenguel, *Improvements on the computation of
  boundaries in QFT*. International Journal of Robust and Nonlinear Control
  16(12):575–597, 2006. DOI 10.1002/rnc.1078. Copyright 2006 John Wiley & Sons,
  Ltd. The method the PFC boundary algorithm follows.
- Y. Chait and O. Yaniv, *Multi-input/single-output computer-aided control design
  using the quantitative feedback theory*. International Journal of Robust and
  Nonlinear Control 3(1):47–54, 1993. Copyright 1993 John Wiley & Sons, Ltd.
  Background on boundary computation.

**Benchmarks**

- C. Borghesani, Y. Chait and O. Yaniv, *The QFT Frequency Domain Control Design
  Toolbox for use with MATLAB*, user's guide. Example 2 of the manual is the
  `qft_toolbox_ex2.qft` fixture.
- B. Wie and D. S. Bernstein, *Benchmark problems for robust control design*,
  American Control Conference, 1990. The ACC'90 benchmark is the `acc90.qft`
  fixture.
