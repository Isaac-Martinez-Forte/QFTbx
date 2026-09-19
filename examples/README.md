# Design problems from the literature, solved

Each problem here is a published QFT design. The input data - the plant and
its uncertainty, the specifications, the design frequencies and the structure
the controller is looked for in - is the paper's; everything else in the file
is computed by QFTbx: the templates, their contours, the boundaries, the
controller and the verifier's verdict on it.

They are not reproductions of the published controllers. The point is to run
each problem and see what this toolbox makes of it, with the paper beside it
to compare against. Where our answer differs from the published one the
problem's own page says so and why.

Open any of them with QFTbx, or rebuild them from the command line:

    qftbx-solve examples/dcm-k.qft -o solved.qft -a mc2

`examples/generate/make_problems.py` writes the inputs of every problem from
the data in the table below, so the battery can be rebuilt from nothing.

## What the toolbox can and cannot take from a paper

The loop shaping searches over controllers with **real zeros and real poles
and a gain**, and that is all. A published structure that has a complex pole
pair, an integrator, a PID in its own parameters or a fractional order cannot
be handed to it as it stands. Those problems are still here where the plant
and the specifications are reproducible: what changes is the structure the
controller is looked for in, and the problem's page says exactly what was
changed and against what published structure the answer should be read.

Lifting that limit is work on the algorithms, not on these files.

## The problems

The plants are grouped by the family they come from. `k a/(s(s+a))` and
`k/(s(s+a))` are the same DC motor written two ways, and which of the two a
paper uses changes the templates - it is not a typo in one of them.

| Problem | Source | Plant and uncertainty | Specifications | Design frequencies | Published controller | In QFTbx |
|---|---|---|---|---|---|---|
| `toolbox-1` | QFT Toolbox manual (Borghesani, Chait and Yaniv), example 1 | `k/((s+a)(s+b))`; `k ∈ [1,10]`, `a ∈ [1,5]`, `b ∈ [20,30]` | stability `1.2`; output disturbance `0.02(s³+64s²+748s+2400)/(s²+14.4s+169)` over [0,10]; input disturbance `0.01` over [0,50] | 0.1 5 10 100 | designed by hand in the manual | yes |
| `toolbox-2` | QFT Toolbox manual, example 2 - the problem most of the rest descend from | `k a/(s(s+a))`; `k,a ∈ [1,10]` | stability `1.2`; corridor `120/(s³+17s²+82s+120)` to `0.6584(s+30)/(s²+4s+19.752)` | 0.1 0.5 1 2 15 100 | designed by hand in the manual; second order by genetic algorithm in Chen, Ballance and Gawthrop 1998 (`k_hf` 136.76 dB); CRONE-2 in Cervera and Baños 2008 (`K_hf` 129.75 dB) | yes |
| `acc90` | ACC'90 benchmark, spring-mass; QFT Toolbox manual example 5 (margin 2.25); Nataraj and Kubal, IJRNC 17 (2007), ex. 4.1 (p. 262-263) | `e/(s²(s²+0.02s+2e))`; `e ∈ [0.5,2]` — a double integrator | stability `1.75` and nothing else, at the margin and the frequencies the QFTbx thesis uses | 0.1 0.98 0.99 1 2 5 7 8.5 10 15 20 100 | Nataraj and Kubal, for their margin specifications: `1.139e7(s+0.0751)(s+0.3488)(s+0.3868)/((s+7.2019)(s+39.7899)(s+95.8659)(s+96.2539))`. Here: one zero, one pole, `k ∈ [1000,10⁸]`, as in the thesis | yes |
| `dcm-k` | Tharewal 2005, ex. 3.1 (p. 37-38) = Nataraj and Tharewal, ASME 2007, ex. 5.1 | `k/(s(s+a))`, `k,a ∈ [1,10]` | stability `1.2`; tracking `T_U = 0.6584(s+30)/(s²+4s+19.752)`, `T_L = 120/(s³+17s²+82s+120)` | 0.1 0.5 1 15 100 | `3462219(s+3.85)/((s+931.27)(s+946.83))` — 1 zero, 2 poles | yes |
| `dcm-ka-w5` | Chait, Chen and Hollot, ASME JDSMC 121 (1999) | `k a/(s(s+a))`, `k,a ∈ [1,10]` | the same as `dcm-k` | 0.1 0.5 1 15 100 | three degrees of freedom by linear programming | structure differs |
| `dcm-ka-w8` | Purohit, Nataraj, Chabert and Goldsztejn, IJRNC 2016, exp. 4.1 (p. 10-12) | `k a/(s(s+a))`, `k,a ∈ [1,10]` | stability `1.2`; the same corridor | 0.5 1 2 3 5 10 30 60 | PID `Kp 9.22, Td 0.41, Ti 12.21` and a prefilter; **the paper admits it violates the upper tracking bound over ω ∈ [11,29]** | structure differs |
| `dcm-AC` | IFAC DYCOPS 2013 (p. 431-432); Tharewal 2005, ex. 3.2 | `k a/(s(s+a))`, `k,a ∈ [1,10]` | stability `1.2`; tracking with the fourth-order lower bound `8400/((s+3)(s+4)(s+10)(s+70))` | 0.5 1 2 10 30 60 | PID `7.03 + 3.89s + 0.1/s` | structure differs |
| `dcm-T33` | Tharewal 2005, ex. 3.3 (p. 40-41) = ASME 2007, ex. 5.3 | `k/(s(s+a))`, `k,a ∈ [1,10]` | stability `1.2`; tracking `T_U = 1.5/(s+1.5)`, `T_L = 1/(s+1)²` | 0.001 0.0157 0.2449 3.8337 60 | `10455(s+1.56)(s+1.29)/((s+0.54)(s²+149.4s+17260))` — **a complex pole pair** | structure differs |
| `dcm-hs72` | Bryant and Halikias 1995, over Horowitz and Sidi 1972 | `k a/(s(s+a))`, `k,a ∈ [1,10]` | the corridor `1/(s+1)² ≤ T ≤ 1.5/(s+1.5)`; the paper states no margin, `1.2` added here as Tharewal 3.3 does | 23 logarithmic, 0.01 to 428.1 | by linear programming | structure differs |
| `aircraft` | Tharewal 2005, ex. 3.5 (p. 43-45) = ASME 2007, ex. 5.5, and Nandkishor 2006, ex. 5.2 (p. 71-73), both over Thompson and Nwokah 1994 | `k(1+s/a)/(s(1+s/b)(1+2ζs/ωn+s²/ωn²))`; `k ∈ [0.2,2]`, `a ∈ [0.5,0.75]`, `b ∈ [1,10]`, `ωn ∈ [5,6]`, `ζ ∈ [0.8,0.9]` | stability **6 dB** (Nandkishor; the ASME paper says 3 dB, and Thompson and Nwokah settle it at 6); tracking `T_U = (1+s/0.35)/((1+s/0.5)(1+s/3))`, `T_L = 1/((1+s)²(1+s/5))` | 0.01 0.05 0.1 0.2 1 5 10 | Tharewal: `190.35(s+13.7)(s+13.67)/((s+96.09)(s+95.7))`, 2 zeros and 2 poles searched in `(0,10⁸]×(0,5000]⁴`; Nandkishor: `42.36(s+3.88)(s+4.46)(s+6.37)/((s+0.66)(s+25.02)(s+58.68))`. **Tharewal's boundaries were computed from rectangular template approximations**, his own words, so his gain is not to be read against ours | yes, as Tharewal poses it |
| `aircraft-w9` | García-Sanz and Guillén 2000 | the same aircraft | the same, 6 dB | 0.01 0.05 0.1 0.2 1 5 10 50 100 | by genetic algorithm | structure differs |
| `flight22` | Purohit et al., IJRNC 2016, exp. 4.4 (p. 16-18) | the same aircraft, 243 plants | stability `6 dB`; corridor — **the published labels are inverted and the corridor is empty at high frequency** | 22, from 0.01 to 300 | fourth order over fourth order, and a prefilter | structure differs |
| `msf` | Tharewal 2005, ex. 4.4 (p. 68-72) | MSF desalination `K(1+T1 s)/((1+T2 s)(1+T3 s))`; `K ∈ [32,76]`, `T1 ∈ [12,28]`, `T2 ∈ [11,26]`, `T3 ∈ [4,10]` | stability `1.2`; tracking added by the authors ("Ismail does not use any tracking specifications") | 0.01 0.098 0.309 0.97 9.558 30 | `655.65(s+2.022)/(s(s+169.9))` — **an integrator** | structure differs |
| `maglev-lower` | Purohit et al., IJRNC 2016, exp. 4.2 (p. 12-13) | `k/(s²+a)`; `k ∈ [811,944]`, `a ∈ [382,478.5]` | stability `1.2`; corridor `916.3/(s³+39.76s²+354.9s+916.3)` to `(1.722s+68.89)/(s²+16.6s+68.89)` | 0.1 1 1.5 2 2.5 3 3.66 5.5 10 20 30 | PID with `ωn`, `ζ` | structure differs |
| `maglev-upper` | the same, the unstable half | `k/(s²−a)`; `k ∈ [1021,1106]`, `a ∈ [382,478.5]` | the same | the same | PID with `ωn`, `ζ` | structure differs |
| `fopdt` | Purohit et al., IJRNC 2016, exp. 4.3 (p. 15-16) | first order plus delay, first-order Padé: `k(1−td s/2)/((s+a)(1+td s/2))`; `k ∈ [1,3]`, `a ∈ [1,2]`, `td ∈ [0.08,0.12]` | stability `1.2`; corridor `9/(s³+7s²+15s+9)` to `4/(s²+3.3s+4)` | 0.1 0.2 0.5 1 2 5 8 10 50 | PID `Kp 1.88, Td 0.05, Ti 0.72` | structure differs |
| `unstable` | Tharewal 2005, ex. 3.6 (p. 47-48) | unstable `k(s+a)/(s²−2.5)`; `k ∈ [1,10]`, `a ∈ [0.1,1]` | stability `2.1` and nothing else ("the only design spec considered") | 0.1 1 2 6 50 | `5.18` — a static gain, searched in `(0,10⁸]`; Chen and Ballance's hand design, `6.582` | **structure differs, and with a finding.** The closed loop's characteristic polynomial is `s² + kC s + (kC a − 2.5)`, stable only where `kC a > 2.5`: **the published 5.18 leaves every member with `a < 0.48` unstable** (6.582, every one with `a < 0.38`), and no static gain below 25 stabilises the worst member, `k = 1, a = 0.1`. The margin at the five design frequencies does not see this - it is met by gains from about 20 - which is the gap between a margin sampled at the design frequencies and the stability of every member, and why the controller here is checked against the closed-loop poles of the whole family and not only against the specifications. It is looked for with one zero and one pole: `0.0738(s+1000)/(s+0.968)`, which stabilises all 81 members of the 9 × 9 family with 0.008 dB to spare on the margin |

## How they were solved

The same way, all of them, so that the files can be read against one another:

- **templates**: every uncertain parameter swept at the same number of points, chosen so that a template comes to about 625 points whatever the number of parameters (25 on each of two, 9 on each of three, 5 on each of four); the ACC'90 benchmark, whose one parameter gives a curve, at 25;
- **contours**: the epsilon each template asks for, worked out by the solver itself;
- **boundaries**: the Nichols grid of 361 × 441 points over phase [−360°, 0] and magnitude [−60, 160] dB, from the contours;
- **loop shaping**: MC2, at the tolerance each problem's page states - 0.5 where it reaches, and a looser one where the search does not finish at 0.5 - and the controller structure of the table, with the box of the paper where the paper gives one;
- **verification**: every controller checked against every specification at every design frequency, and the worst excess recorded. A file is only here if that excess is negative.

All of it is in the file: open one and each phase shows what it was computed with.

## What is not here, and why

The corpus these come from has more problems than the toolbox can take. Left
out, with the reason:

- **fractional-order plants or controllers** (Tharewal 4.1, 4.2, 4.3; Nandkishor 5.4; Nataraj and Kalla 2010): the toolbox has no `s^μ`.
- **multi-input multi-output** (Patil and Nataraj 2012, the 2×2 maglev).
- **non-parametric uncertainty** (IFAC 2008 ex. 2; Deshpande and Nataraj 2015 ex. 2), where the family is a magnitude envelope and not a box of parameters.
- **H∞ criteria** (Tharewal 5.1), where two specifications are added rather than intersected.
- **fixed plants with a bandwidth specification** (IFAC 2008 ex. 1; DYCOPS 2013 §3.4; Deshpande and Nataraj 2015 ex. 1): no uncertainty, so no templates.
- **experimental setups** (Jeyasenthil and Nataraj 2015 ex. 2), whose uncertainty ranges are not published.
- **prefilter design** (Tharewal, chapter 7): the toolbox has no prefilter.
- **Tharewal 6.2**, the jet engine `43/(s³+as²+bs)`: as transcribed from the PDF, the tracking corridor is narrower than the template below 0.06 rad/s and the problem is infeasible whatever the structure and whatever the margin - with the paper's own `γ = 1.001` and with `1.2`. The plant's two uncertain coefficients and the corridor are marked as doubtful readings in the corpus; until somebody checks them against the original the problem stays out.
- **Tharewal 6.1**, where the paper's own answer is that no feasible solution exists - worth returning to as a test of the infeasibility verdict rather than as a design.

Two of the QFT Toolbox manual's examples are reproducible but not yet done:
**example 9**, the flexible Philips mechanism, whose uncertainty is in a
coefficient of the numerator and whose margin comes with weights, and
**example 10**, an inverted pendulum, which is unstable and has a delay. Of
the other ten, example 3 has non-parametric uncertainty, 4 a fixed plant, 6 a
missile in three cases with an unstable controller, 7, 8 and 15 are multiloop,
11 is an experimental response and 12 to 14 are discrete-time.
