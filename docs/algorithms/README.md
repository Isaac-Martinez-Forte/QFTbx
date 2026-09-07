# The algorithms of QFTbx

QFTbx is a research toolbox. Every computation it performs comes from a published
piece of work, and this folder is where each one is explained: what problem it
solves, where the method comes from, how the implementation follows the reference
and where it deliberately departs from it. The pages are written for a reader who
knows control engineering but has never opened the code; each one ends with the
files that implement the algorithm and the tests that pin it down.

## Context

Quantitative Feedback Theory (QFT) is a frequency-domain method for designing
robust controllers for plants with parametric uncertainty. A design walks through
a fixed sequence of steps, and QFTbx implements every one of them:

```
 plant + parametric     design           templates          specifications
 uncertainty        →   frequencies  →   (value sets +  →   (stability,
                        (ω vector)       ε-hull contour)    tracking, ...)
                                                                 │
                        controller   ←   automatic loop  ←   boundaries
                        structure        shaping             (Nichols plane)
```

1. **Templates.** At each design frequency the uncertain plant is not one point of
   the Nichols chart but a set, the template. QFTbx sweeps the uncertainty and
   reduces the resulting cloud to its contour.
2. **Boundaries.** Each specification (robust stability, tracking, disturbance
   rejection, control effort) translates, frequency by frequency, into a curve of
   the Nichols chart that the nominal open loop must stay on one side of. The
   curves of all the specifications merge into one boundary per frequency.
3. **Automatic loop shaping.** Finding a controller of a fixed structure that
   satisfies every boundary with the least high-frequency gain is a nonconvex
   global optimisation problem. QFTbx solves it with interval branch & bound
   algorithms, which certify the optimum rather than sample for it.

The toolbox grew with the academic work of its author. The template and boundary
algorithms were specified and prototyped in the 2013 degree project (PFC), their
parallel CPU and GPU versions are the 2014 master's thesis (TFM), and the loop
shaping algorithms are the subject of the 2022 doctoral thesis and the 2021 paper
in the International Journal of Robust and Nonlinear Control. The loop-shaping
family builds on the interval algorithms of P. S. V. Nataraj's group at IIT Bombay,
which QFTbx also implements so that the accelerations can be measured against
their baselines.

## The pages

| Step | Page | Algorithm | Origin |
|---|---|---|---|
| Templates | [templates.md](templates.md) | Brute-force sweep and ε-hull contour | Nordin 1993 in Montoya's 1998 implementation; PFC 2013; TFM 2014 (CUDA) |
| Boundaries | [boundaries.md](boundaries.md) | Sheets on the Nichols grid, level-curve tracing, 1D union | PFC 2013 (after Moreno et al. 2006); TFM 2014 (CUDA) |
| Loop shaping | [loop-shaping.md](loop-shaping.md) | The common machinery: interval extension, box classification, nominal stability | Tharewal 2005; thesis 2022 ch. 1 and 3 |
| Loop shaping | [loop-shaping-nt.md](loop-shaping-nt.md) | **NT**, the base interval branch & bound | Nataraj and Tharewal; Tharewal 2005 |
| Loop shaping | [loop-shaping-nk.md](loop-shaping-nk.md) | **NK**, NT with Quick Solution cuts and local optimisation | Nataraj and Kubal 2007 |
| Loop shaping | [loop-shaping-mr.md](loop-shaping-mr.md) | **MR**, synthesis as an interval constraint satisfaction problem | Kalla and Nataraj 2010 |
| Loop shaping | [loop-shaping-mc1.md](loop-shaping-mc1.md) | **MC (2021)**, NT/NK accelerated with phase and feasible-box information | Martínez-Forte and Cervera 2021 |
| Loop shaping | [loop-shaping-mc.md](loop-shaping-mc.md) | **MC (thesis)**, every strategy of the thesis assembled | Martínez-Forte 2022, ch. 4 and 5 |

The genealogy of the loop-shaping algorithms, which the pages follow:

```
NT  Nataraj and Tharewal (2002-2005)      interval branch & bound, gain cuts
 └─ NK  Nataraj and Kubal (2007)          + Quick Solution cuts on every parameter, local optimisation
     ├─ MR  Kalla and Nataraj (2010)      constraint satisfaction: HC4 filtering instead of feasibility tests
     └─ MC  Martínez-Forte and Cervera (2021)   + phase information, feasible-box information
         └─ MC  Martínez-Forte (2022)     + feasible subranges, best-gain search, tree bisection, stages
```

## The documents

The [references](references/README.md) page holds the full bibliography. The
author's own documents are in the [references](references/) folder as PDF files:
the degree project, the master's thesis and the doctoral thesis; the 2021 paper
is read in open access from the university repository. The third-party papers
and theses are cited with their DOI or original location; their copyright
belongs to their publishers or authors and they are not redistributed here. The
references page says, for each document, which is which.

## Conventions

- Frequencies are in rad/s, magnitudes in dB and phases in degrees on the Nichols
  branch (-360°, 0], unless a page says otherwise.
- The nominal plant is P0 and the nominal open loop is L0 = P0 C, with C the
  controller being designed.
- "Certainly feasible", "certainly infeasible" and "ambiguous" are the three
  answers of the interval feasibility test; they are certificates, not estimates.
- Where the implementation departs from its reference, the page says so under a
  heading of its own, and the same note lives in the header of the class. The
  errata found in the references while reviewing the implementations are listed
  on the page of the algorithm they concern.
