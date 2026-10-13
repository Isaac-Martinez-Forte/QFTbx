# Test data

Sample `.qft` project files used as golden data by the unit tests.

| File | Source | Sections present |
|---|---|---|
| `cervera.qft` | ACC example, Cervera & Baños 2013 | plant, frequencies |
| `planta2.qft` | Example plant 2 | plant, frequencies, specifications, templates (full + contour) |
| `multivaluados.qft` | Moreno & Baños (multivalued boundaries) | full project up to boundaries + controller structure |
| `planta1.qft` | Example plant 1 | full project including loop shaping |
| `qft_toolbox_ex2.qft` | Matlab QFT Toolbox design example 2 (thesis chapter 6) | full project up to boundaries + n = 3 controller structure |
| `acc90.qft` | ACC'90 benchmark (thesis chapter 6); resonant poles lightly damped (+0.02 s) | full project up to boundaries + n = 3 controller structure |
| `dcm-T33-plant-k.qft` | The DC motor written `k/(s(s+a))`, not the `k a/(s(s+a))` of the papers, with the corridor and margin of Tharewal 3.3 | full project including loop shaping |
| `dcm-k-plant-k.qft` | The same motor with the specifications of Tharewal 3.1 | full project including loop shaping |
| `acc90-damped.qft` | The ACC'90 benchmark with its resonance lightly damped (+0.02 s), the margin of 1.75 alone and one zero and two poles from a gain of 1000 | full project including loop shaping |

`corrupt_*.qft` and `invalid.qft` are hand-made malformed inputs for the
reader's error paths. The two `*-plant-k.qft` problems and `acc90-damped.qft` are
test problems: the tests that pin gains, boxes and designs on them were written
against those plants, which differ from the ones the papers print (the benchmark's
resonance is undamped). The two thesis benchmarks were generated through the
real pipeline (template grids 25x25 uniform and 80-point cosine-spaced
respectively; boundaries on (-360,0)x361 with magnitudes (-60,160)x441 for
example 2 - its low-frequency tracking bound sits at +37..57 dB and the
published optimum hugs the bounds within tenths of a dB, so the fixture
resolution matters - and (-60,60)x121 for ACC'90). The tracking models follow Tharewal 2005 Example
3.1 (T_U the second-order model, T_L the third-order one) and the search
boxes are sized so the honest branch & bound stays tractable.
