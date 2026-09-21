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

Open any of them with QFTbx, or solve one again from the command line - the
file carries every input and setting the result was computed with:

    qftbx-solve examples/dcm-k.qft -o solved.qft -a mc2

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
| `toolbox-1` | QFT Toolbox manual (Borghesani, Chait and Yaniv), example 1 | `k/((s+a)(s+b))`; `k ∈ [1,10]`, `a ∈ [1,5]`, `b ∈ [20,30]` | stability `1.2`; output disturbance `0.02(s³+64s²+748s+2400)/(s²+14.4s+169)` over [0,10]; input disturbance `0.01` over [0,50] | 0.1 5 10 100 | designed by hand in the manual | yes, ε = 0.5 |
| `toolbox-2` | QFT Toolbox manual, example 2 - the problem most of the rest descend from | `k a/(s(s+a))`; `k,a ∈ [1,10]` | stability `1.2`; corridor `120/(s³+17s²+82s+120)` to `0.6584(s+30)/(s²+4s+19.752)` | 0.1 0.5 1 2 15 100 | designed by hand in the manual; second order by genetic algorithm in Chen, Ballance and Gawthrop 1998 (`k_hf` 136.76 dB); CRONE-2 in Cervera and Baños 2008 (`K_hf` 129.75 dB) | yes, ε = 0.5 |
| `acc90` | ACC'90 benchmark, spring-mass; QFT Toolbox manual example 5 (margin 2.25); Nataraj and Kubal, IJRNC 17 (2007), ex. 4.1 (p. 262-263) | `e/(s²(s²+0.02s+2e))`; `e ∈ [0.5,2]` — a double integrator | stability `1.75` and nothing else, at the margin and the frequencies the QFTbx thesis uses | 0.1 0.98 0.99 1 2 5 7 8.5 10 15 20 100 | Nataraj and Kubal, for their margin specifications: `1.139e7(s+0.0751)(s+0.3488)(s+0.3868)/((s+7.2019)(s+39.7899)(s+95.8659)(s+96.2539))`. Here: one zero, one pole, `k ∈ [1000,10⁸]`, as in the thesis | yes, ε = 0.5 |
| `dcm-k` | Tharewal 2005, ex. 3.1 (p. 37-38) = Nataraj and Tharewal, ASME 2007, ex. 5.1 | `k/(s(s+a))`, `k,a ∈ [1,10]` | stability `1.2`; tracking `T_U = 0.6584(s+30)/(s²+4s+19.752)`, `T_L = 120/(s³+17s²+82s+120)` | 0.1 0.5 1 15 100 | `3462219(s+3.85)/((s+931.27)(s+946.83))` — 1 zero, 2 poles | yes, ε = 0.5 |
| `dcm-ka-w5` | Chait, Chen and Hollot, ASME JDSMC 121 (1999) | `k a/(s(s+a))`, `k,a ∈ [1,10]` | the same as `dcm-k` | 0.1 0.5 1 15 100 | three degrees of freedom by linear programming | yes, ε = 0.5 |
| `dcm-ka-w8` | Purohit, Nataraj, Chabert and Goldsztejn, IJRNC 2016, exp. 4.1 (p. 10-12) | `k a/(s(s+a))`, `k,a ∈ [1,10]` | stability `1.2`; the same corridor | 0.5 1 2 3 5 10 30 60 | PID `Kp 9.22, Td 0.41, Ti 12.21` and a prefilter; **the paper admits it violates the upper tracking bound over ω ∈ [11,29]** | yes, ε = 0.5 |
| `dcm-AC` | IFAC DYCOPS 2013 (p. 431-432); Tharewal 2005, ex. 3.2 | `k a/(s(s+a))`, `k,a ∈ [1,10]` | stability `1.2`; tracking with the fourth-order lower bound `8400/((s+3)(s+4)(s+10)(s+70))` | 0.5 1 2 10 30 60 | PID `7.03 + 3.89s + 0.1/s` | yes, ε = 0.5 |
| `dcm-T33` | Tharewal 2005, ex. 3.3 (p. 40-41) = ASME 2007, ex. 5.3 | `k/(s(s+a))`, `k,a ∈ [1,10]` | stability `1.2`; tracking `T_U = 1.5/(s+1.5)`, `T_L = 1/(s+1)²` | 0.001 0.0157 0.2449 3.8337 60 | `10455(s+1.56)(s+1.29)/((s+0.54)(s²+149.4s+17260))` — **a complex pole pair** | **structure differs** - file: one zero, one pole |
| `dcm-hs72` | Bryant and Halikias 1995, over Horowitz and Sidi 1972 | `k a/(s(s+a))`, `k,a ∈ [1,10]` | the corridor `1/(s+1)² ≤ T ≤ 1.5/(s+1.5)`; the paper states no margin, `1.2` added here as Tharewal 3.3 does | 23 logarithmic, 0.01 to 428.1 | by linear programming | **structure differs** - file: one zero, two poles |
| `aircraft` | Tharewal 2005, ex. 3.5 (p. 43-45) = ASME 2007, ex. 5.5, and Nandkishor 2006, ex. 5.2 (p. 71-73), both over Thompson and Nwokah 1994 | `k(1+s/a)/(s(1+s/b)(1+2ζs/ωn+s²/ωn²))`; `k ∈ [0.2,2]`, `a ∈ [0.5,0.75]`, `b ∈ [1,10]`, `ωn ∈ [5,6]`, `ζ ∈ [0.8,0.9]` | stability **6 dB** (Nandkishor; the ASME paper says 3 dB, and Thompson and Nwokah settle it at 6); tracking `T_U = (1+s/0.35)/((1+s/0.5)(1+s/3))`, `T_L = 1/((1+s)²(1+s/5))` | 0.01 0.05 0.1 0.2 1 5 10 | Tharewal: `190.35(s+13.7)(s+13.67)/((s+96.09)(s+95.7))`, 2 zeros and 2 poles searched in `(0,10⁸]×(0,5000]⁴`; Nandkishor: `42.36(s+3.88)(s+4.46)(s+6.37)/((s+0.66)(s+25.02)(s+58.68))`. **Tharewal's boundaries were computed from rectangular template approximations**, his own words, so his gain is not to be read against ours | **not here**: two zeros and two poles, and every smaller structure, ran six hours without finishing or came back infeasible |
| `aircraft-w9` | García-Sanz and Guillén 2000 | the same aircraft | the same, 6 dB | 0.01 0.05 0.1 0.2 1 5 10 50 100 | by genetic algorithm | **not here**: as `aircraft` |
| `flight22` | Purohit et al., IJRNC 2016, exp. 4.4 (p. 16-18) | the same aircraft, 243 plants | stability `6 dB`; corridor — **the published labels are inverted and the corridor is empty at high frequency** | 22, from 0.01 to 300 | fourth order over fourth order, and a prefilter | **not here**: fourth order over fourth order and every smaller structure, six hours without finishing |
| `msf` | Tharewal 2005, ex. 4.4 (p. 68-72) | MSF desalination `K(1+T1 s)/((1+T2 s)(1+T3 s))`; `K ∈ [32,76]`, `T1 ∈ [12,28]`, `T2 ∈ [11,26]`, `T3 ∈ [4,10]` | stability `1.2`; tracking added by the authors ("Ismail does not use any tracking specifications") | 0.01 0.098 0.309 0.97 9.558 30 | `655.65(s+2.022)/(s(s+169.9))` — **an integrator** | yes, ε = 0.5 |
| `maglev-lower` | Purohit et al., IJRNC 2016, exp. 4.2 (p. 12-13) | `k/(s²+a)`; `k ∈ [811,944]`, `a ∈ [382,478.5]` | stability `1.2`; corridor `916.3/(s³+39.76s²+354.9s+916.3)` to `(1.722s+68.89)/(s²+16.6s+68.89)` | 0.1 1 1.5 2 2.5 3 3.66 5.5 10 20 30 | PID with `ωn`, `ζ` | **not here**: the paper's order and ours ran six hours without finishing; one zero and two poles returns no controller |
| `maglev-upper` | the same, the unstable half | `k/(s²−a)`; `k ∈ [1021,1106]`, `a ∈ [382,478.5]` | the same | the same | PID with `ωn`, `ζ` | **structure differs** - file: one zero, one pole |
| `fopdt` | Purohit et al., IJRNC 2016, exp. 4.3 (p. 15-16) | first order plus delay, first-order Padé: `k(1−td s/2)/((s+a)(1+td s/2))`; `k ∈ [1,3]`, `a ∈ [1,2]`, `td ∈ [0.08,0.12]` | stability `1.2`; corridor `9/(s³+7s²+15s+9)` to `4/(s²+3.3s+4)` | 0.1 0.2 0.5 1 2 5 8 10 50 | PID `Kp 1.88, Td 0.05, Ti 0.72` | **not here**: five structures, six hours each, none finished |
| `unstable` | Tharewal 2005, ex. 3.6 (p. 47-48) | unstable `k(s+a)/(s²−2.5)`; `k ∈ [1,10]`, `a ∈ [0.1,1]` | stability `2.1` and nothing else ("the only design spec considered") | 0.1 1 2 6 50 | `5.18` — a static gain, searched in `(0,10⁸]`; Chen and Ballance's hand design, `6.582` | **structure differs, and with a finding.** The closed loop's characteristic polynomial is `s² + kC s + (kC a − 2.5)`, stable only where `kC a > 2.5`: **the published 5.18 leaves every member with `a < 0.48` unstable** (6.582, every one with `a < 0.38`), and no static gain below 25 stabilises the worst member, `k = 1, a = 0.1`. The margin at the five design frequencies does not see this - it is met by gains from about 20 - which is the gap between a margin sampled at the design frequencies and the stability of every member, and why the controller here is checked against the closed-loop poles of the whole family and not only against the specifications. It is looked for with one zero and one pole: `0.0738(s+1000)/(s+0.968)`, which stabilises all 81 members of the 9 × 9 family with 0.008 dB to spare on the margin - file: one zero, one pole |

## How they were solved

The same way, all of them, so that the files can be read against one another:

- **templates**: every uncertain parameter swept at the same number of points, chosen so that a template comes to about 625 points whatever the number of parameters (25 on each of two, 9 on each of three, 5 on each of four); the ACC'90 benchmark, whose one parameter gives a curve, at 25;
- **contours**: the epsilon each template asks for, worked out by the solver itself;
- **boundaries**: the Nichols grid of 361 × 441 points over phase [−360°, 0] and magnitude [−60, 160] dB, from the contours;
- **loop shaping**: MC2 at tolerance **0.5** for every one of the twelve, with the controller structure the table gives and the box of the paper where the paper gives one. Each problem was also run at tolerances 2 and 5 and with three or four other structures; those runs are kept with the internal material and are not here;
- **verification**: every controller checked against every specification at every design frequency, and the worst excess recorded. A file is only here if that excess is negative.

All of it is in the file: open one and each phase shows what it was computed with. Six of the eighteen problems posed are not here because no structure we can search finished in six hours, or none was feasible; they are listed below.

## The problems, one by one

### toolbox-1 — example 1 of the QFT Toolbox manual

**Source.** Borghesani, Chait and Yaniv, the QFT Frequency Domain Control Design Toolbox manual, example 1 - the one the manual walks through first.

**The problem.** `P(s) = k / ((s + a)(s + b))`, `k ∈ [1, 10]`, `a ∈ [1, 5]`, `b ∈ [20, 30]`. Stability 1.2 over [0.1, 100]; output disturbance `|1/(1+L)| ≤ 0.02 (s³+64s²+748s+2400)/(s²+14.4s+169)` over [0.1, 10]; input disturbance `|P/(1+L)| ≤ 0.01` over [0.1, 50]; ω = 0.1, 5, 10, 100 rad/s.

**Published controller.** Designed by hand in the manual's loop-shaping environment; the QFTbx thesis searches a gain over the fixed structure `(s/42+1)/(s/165+1)`.

**How it is posed here.** One zero, one pole in `[0.01, 1000]`, `k ∈ [1, 10⁶]`.

**What came out.** ε = 0.5: **gain 20.63**, worst excess −0.039 dB over the three specifications at their frequencies.

**How to read it.** The one problem of the battery with disturbance specifications; the plant is stable and has no integrator, so it is also the one where the gain is not pushed to a box edge by the low-frequency tracking.

### toolbox-2 — example 2 of the QFT Toolbox manual

**Source.** Borghesani, Chait and Yaniv, the QFT Toolbox manual, example 2. Half the DC motors of this battery descend from it, at other frequencies or with other bounds; this is the original.

**The problem.** `P(s) = k a / (s (s + a))`, `k, a ∈ [1, 10]`; stability 1.2; tracking between `120/(s³+17s²+82s+120)` and `0.6584 (s+30)/(s²+4s+19.752)`; ω = 0.1, 0.5, 1, 2, 15, 100 rad/s.

**Published controllers.** The manual's, by hand. Chen, Ballance and Gawthrop 1998, second order by genetic algorithm, `k_hf` 136.76 dB. Cervera and Baños 2008, CRONE-2 with `n_pe = 3`, `K_hf` 129.75 dB. Chen et al. print the coefficient as 0.6854 and "828" for 82 - readings of the manual's 0.6584 and 82.

**How it is posed here.** One zero, one pole in `[0.01, 1000]`, `k ∈ [1, 10⁶]`, as the QFTbx thesis poses it.

**What came out.** ε = 0.5: **gain 567.32** (55.1 dB), worst excess −0.00007 dB. This is, to the digit, the value the thesis's benchmark pins for MC2 under the conservative reading of the boundary columns, which is how the battery files are computed.

**How to read it.** The published `k_hf` figures are high-frequency gains of second-order controllers with their own tolerances; the number here is the high-frequency gain of a first-order one. What the file shows is where the boundaries put the loop of this family; the gap to the published designs is a question about their structures and tolerances, not about this file.

### acc90 — the ACC'90 spring-mass benchmark

**Source.** The ACC'90 benchmark problem; example 5 of the QFT Toolbox manual (Borghesani, Chait and Yaniv), where it is designed with a margin of 2.25; Nataraj and Kubal, IJRNC 17 (2007), ex. 4.1, p. 262-263, with margin specifications of their own. Here it is posed as the QFTbx thesis poses it.

**The problem.** `P(s) = e / (s² (s² + 0.02 s + 2e))`, `e ∈ [0.5, 2]`: two masses joined by an uncertain spring, a double integrator, a lightly damped pair at `√(2e)`. Stability margin **1.75** and nothing else, at ω = 0.1, 0.98, 0.99, 1, 2, 5, 7, 8.5, 10, 15, 20 and 100 rad/s. The three frequencies around 1 rad/s straddle the resonance, which sweeps from 1 to 2 rad/s across the family.

**Published controller.** Nataraj and Kubal, for their margins: `1.139·10⁷ (s+0.0751)(s+0.3488)(s+0.3868) / ((s+7.2019)(s+39.7899)(s+95.8659)(s+96.2539))`, three zeros and four poles. The QFTbx thesis uses one zero, one pole and `k ∈ [1000, 10⁸]`, and that is the structure here.

**What came out.** One zero, one pole, ε = 0.5: **gain 1000**, the bottom of the box, worst excess **−4.85 dB**. The same gain and margin at ε = 2 and 5 and with 1-2 and 2-2 structures; with 2 zeros and 3 poles the margin opens to −66.8 dB at the same gain. The gain sits on the lower edge of the box because the template is a curve - one uncertain parameter - and the margin is met with room to spare all the way down; the box's lower edge is the thesis's, kept so that the file matches its goldens.

**How to read it.** The template of a one-parameter family is a curve, and its ε-hull is walked out and back, so the contour has more points than the cloud; that is expected, not a defect. `e` was swept at 25 points, as in the thesis.

### dcm-k — the DC motor of Tharewal 3.1

**Source.** Tharewal 2005, example 3.1 (p. 37-38), the same as Nataraj and Tharewal, ASME JDSMC 2007, example 5.1; the design Chen, Ballance and Gawthrop 1998 made with a genetic algorithm is what it is compared against there.

**The problem.** `P(s) = k / (s (s + a))`, `k, a ∈ [1, 10]` - note **`k`, not `k·a`** in the numerator: this is Tharewal's plant, and its templates are not those of the QFT Toolbox's example 2 although the specifications are. Stability margin 1.2; tracking between `T_L = 120/(s³+17s²+82s+120)` and `T_U = 0.6584 (s+30)/(s²+4s+19.752)`, at ω = 0.1, 0.5, 1, 15 and 100 rad/s.

**Published controller.** `3462219 (s+3.85) / ((s+931.27)(s+946.83))`: one zero, two poles, a high-frequency gain of 3.46·10⁶, "a 48.73 % reduction" against Chen et al.

**How it is posed here.** The paper's structure: one zero and two poles, `k ∈ [0.01, 10⁸]`, zeros and poles in `[0.01, 1000]`.

**What came out.** ε = 0.5: **gain 49 419**, worst excess −0.0005 dB. The same structure at ε = 2 and 5 solves too; so do one zero-one pole and two-two.

**How to read it.** Both controllers are zero-pole-gain forms with one more pole than zero, so the gain of each is its high-frequency gain and the two numbers are comparable in kind: 4.9·10⁴ here against 3.5·10⁶ published, with the reservation that the paper's boundaries were read from its own templates and tolerance, and ours from exact templates, contours and a 361-column phase grid. The tracking bound's coefficient is the correct **0.6584**; several papers carry 0.6854, a transposition that travels from one to the next.

### dcm-ka-w5 — the QFT Toolbox motor at Chait's frequencies

**Source.** Chait, Chen and Hollot, "Automatic loop-shaping of QFT controllers via linear programming", ASME JDSMC 121 (1999): the QFT Toolbox's example 2 at five design frequencies instead of six.

**The problem.** `P(s) = k a / (s (s + a))`, `k, a ∈ [1, 10]`; stability 1.2; tracking between `120/(s³+17s²+82s+120)` and `0.6584 (s+30)/(s²+4s+19.752)`; ω = 0.1, 0.5, 1, 15, 100 rad/s. Against `toolbox-2` the only change is the missing 2 rad/s.

**Published controller.** A controller of three degrees of freedom found by linear programming; the paper writes the tracking coefficient as 0.6854, a transposition of the Toolbox manual's 0.6584 - the one used here.

**How it is posed here.** One zero, one pole - three degrees of freedom - in `[0.01, 1000]`, `k ∈ [0.01, 10⁸]`.

**What came out.** ε = 0.5: **gain 569.78**, worst excess −0.0003 dB; at ε = 2 and 5 as well, and with every larger structure tried.

**How to read it.** Next to `toolbox-2` (gain 567.32 with the sixth frequency): dropping 2 rad/s costs nothing here, the binding frequencies being elsewhere.

### dcm-ka-w8 — the QFT Toolbox motor at Purohit's eight frequencies

**Source.** Purohit, Nataraj, Chabert and Goldsztejn, IJRNC 2016, experiment 4.1 (p. 10-12).

**The problem.** `P(s) = k a / (s (s + a))`, `k, a ∈ [1, 10]`, which the paper samples at 5 × 5 = 25 plants; stability 1.2; tracking between `120/(s³+17s²+82s+120)` and `0.6585 (s+30)/(s²+4s+19.752)`; ω = 0.5, 1, 2, 3, 5, 10, 30, 60 rad/s.

**Published controller.** A PID, `Kp = 9.22`, `Td = 0.41`, `Ti = 12.21`, with a prefilter. **The paper itself records that it violates the upper tracking bound over ω ∈ [11, 29] rad/s.**

**How it is posed here.** A PID is two zeros over an integrator; the search here has no integrator, so it looks for two zeros and one real pole in `[0.01, 1000]`, `k ∈ [0.01, 10⁸]`. Without the integrator the steady-state tracking error is not zero, which the PID's is; the comparison is of the frequency-domain design, not of the step response.

**What came out.** ε = 0.5: **gain 0.3336**, worst excess −0.0078 dB, every specification met at the eight frequencies. At ε = 2 and 5, and with the 1-1, 1-2 and 2-2 structures, as well.

**How to read it.** For a two-zero, one-pole controller the gain is the coefficient of `s` at high frequency, the counterpart of the PID's `Kd = Kp·Td = 3.78`.

### dcm-AC — the DC motor with the fourth-order lower bound

**Source.** IFAC DYCOPS 2013 (p. 431-432), the same problem as Tharewal 2005 example 3.2 (p. 38-39), against the optimal PID of Zolotas and Halikias.

**The problem.** `P(s) = k a / (s (s + a))`, `k, a ∈ [1, 10]`; stability 1.2; tracking between the fourth-order `8400/((s+3)(s+4)(s+10)(s+70))` and `0.6584 (s+30)/(s²+4s+19.752)` - the DYCOPS paper prints the two labels the other way round; ω = 0.5, 1, 2, 10, 30, 60 rad/s.

**Published controller.** The PID `7.03 + 3.89 s + 0.1/s`, with `Ki = 0.1` on the lower edge of its box; Tharewal's is `12.1 + 3.53 s + 0.38/s`.

**How it is posed here.** Two zeros and one real pole, the PID's two zeros without its integrator, in `[0.01, 1000]`, `k ∈ [0.01, 10⁸]`.

**What came out.** ε = 0.5: **gain 0.3117**, worst excess −0.0060 dB. At ε = 2 and 5, and with the 1-1, 1-2 and 2-2 structures, as well; two zeros and three poles did not finish in six hours at any tolerance.

**How to read it.** The gain is the coefficient of `s` at high frequency, against the PID's `Kd = 3.89`.

### dcm-T33 — Tharewal 3.3, first-order bounds at very low frequencies

**Source.** Tharewal 2005, example 3.3 (p. 40-41) = ASME JDSMC 2007, example 5.3, against the linear-programming design of Bryant and Halikias.

**The problem.** `P(s) = k / (s (s + a))`, `k, a ∈ [1, 10]`; stability 1.2; tracking between `1/(s+1)²` and `1.5/(s+1.5)`; ω = 0.001, 0.0157, 0.2449, 3.8337 and 60 rad/s - three decades below 1 rad/s, where the bounds are nearly flat.

**Published controller.** `10455 (s+1.56)(s+1.29) / ((s+0.54)(s²+149.4s+17260))`, two zeros, one real pole and a complex pair, searched in a 30 % neighbourhood of Bryant and Halikias's parameters.

**How it is posed here.** The complex pair cannot be searched for, and two zeros with three real poles did not finish in six hours at any tolerance. The file carries **one zero and one pole**.

**What came out.** ε = 0.5: **gain 37.46**, worst excess 0.0000 dB - the controller sits on the bound. One zero and two poles solve at ε = 2 and 5; two and two only at ε = 5.

**How to read it.** The gain is a high-frequency gain, against the paper's 10 455 - a different structure, so a comparison of kind and not of merit.

### dcm-hs72 — Horowitz and Sidi's corridor, by Bryant and Halikias

**Source.** Bryant and Halikias 1995, over the problem of Horowitz and Sidi 1972; Tharewal 2005 example 3.3 solves the same bounds on the plant `k/(s(s+a))` at five frequencies (`dcm-T33`).

**The problem.** `P(s) = k a / (s (s + a))`, `k, a ∈ [1, 10]`; the corridor `1/(s+1)² ≤ T ≤ 1.5/(s+1.5)`; 23 frequencies spaced logarithmically from 0.01 to 428.1 rad/s. The paper states no stability margin; **1.2 is added here**, as Tharewal does in 3.3.

**Published controller.** `13721 (s+1.21)(s+1) / ((s+0.599)(s²+150s+150²))`: two zeros, one real pole and a complex pair, found by linear programming.

**How it is posed here.** The complex pair cannot be searched for; the paper's order, two zeros and three real poles, did not finish in six hours at any tolerance. The file carries **one zero and two poles**, the smallest of our structures that solved at ε = 0.5.

**What came out.** ε = 0.5: **gain 27 396**, worst excess −0.0003 dB. Two zeros and two poles solve at every tolerance too; one zero and one pole only at ε = 5 (gain 70.88).

**How to read it.** Twenty-three frequencies is the longest set of the battery, and the boundaries are the most expensive to compute; the search with the paper's order is what does not end. The gain, one more pole than zero, is a high-frequency gain, against the paper's 13 721.

### msf — the MSF desalination plant of Tharewal 4.4

**Source.** Tharewal 2005, example 4.4 (p. 68-72): the top-brine-temperature loop of a multi-stage flash desalination plant, over the model of Ismail. Tharewal uses it for an integer-order against a fractional-order controller.

**The problem.** `P(s) = K (1 + T1 s) / ((1 + T2 s)(1 + T3 s))`, four uncertain parameters: `K ∈ [32, 76]`, `T1 ∈ [12, 28]`, `T2 ∈ [11, 26]`, `T3 ∈ [4, 10]`; nominal `54 (1 + 20.32 s)/((1 + 18.3 s)(1 + 7.2 s))`. Stability margin 1.2; tracking between `60.44/(s³ + 13.5 s² + 50.94 s + 60.44)` and `(16.32 s + 197.9)/(12.12 s² + 64.65 s + 197.9)`, which the authors add themselves ("Ismail does not use any tracking specifications"); ω = 0.01, 0.098, 0.309, 0.97, 9.558, 30 rad/s. A stable plant with no integrator and slow time constants.

**Published controller.** `655.65 (s + 2.022) / (s (s + 169.9))`: one zero, an integrator and one pole, searched in `{[1, 10⁸], [0.001, 50], [0.001, 1500], [0.001, 1500]}`.

**How it is posed here.** One zero and two real poles - the paper's order, without the integrator - in `[0.01, 1000]`, `k ∈ [0.01, 10⁸]`. Four uncertain parameters swept at 5 points each, 625 plants per template.

**What came out.** ε = 0.5: **gain 245.61**, worst excess −0.000001 dB - the controller sits on the tracking bound. The same structure at ε = 2 and 5; one zero and one pole, and two and two, as well. Two zeros and three poles did not finish in six hours.

**How to read it.** With one more pole than zero the gain is a high-frequency gain, against the paper's 655.65 with its integrator. The integrator is what gives the published design zero steady-state error; the design here does not have it, and reads as a frequency-domain comparison only.

### maglev-upper — the unstable half of Purohit's maglev

**Source.** Purohit, Nataraj, Chabert and Goldsztejn, IJRNC 2016, experiment 4.2 (p. 12-13): a magnetic levitation system linearised on either side of its equilibrium. This is the upper, open-loop unstable one; the lower one (`maglev-lower`) did not finish here.

**The problem.** `P(s) = k / (s² − a)`, `k ∈ [1021, 1106]`, `a ∈ [382, 478.5]`: one real pole in the right half-plane at every member, at `√a ≈ 20 rad/s`. Stability margin 1.2; tracking between `916.3/(s³ + 39.76 s² + 354.9 s + 916.3)` and `(1.722 s + 68.89)/(s² + 16.6 s + 68.89)`; eleven frequencies from 0.1 to 30 rad/s.

**Published controller.** A PID with a second-order filter, `Kp = 2.956`, `Td = 0.0604`, `Ti = 0.4195`, `ωn = 5.0075`, `ζ = 0.995`.

**How it is posed here.** A PID with a second-order filter is two zeros over an integrator and a complex pair; the search has neither, and two zeros with three real poles did not finish in six hours. The file carries **one zero and one pole**, the smallest of our structures that solved, in `[0.01, 1000]`, `k ∈ [0.01, 10⁸]`.

**What came out.** ε = 0.5: **gain 715.09**, worst excess −0.00001 dB. Two zeros and two poles solve at every tolerance as well (gain 8.34 at ε = 0.5); one zero and two poles returns no controller.

**How to read it.** The family is unstable in open loop, so the stability of the nominal loop is read with the right half-plane pole counted, as the search does since the plant families with such poles were admitted. A gain of 715 with one zero and one pole is a lead: the zero below the pole, lifting the phase around the crossover the unstable pole forces above 20 rad/s.

### unstable — Tharewal's unstable plant, stability alone

**Source.** Tharewal 2005, example 3.6 (p. 47-48), designed directly on the unstable nominal plant rather than on a stabilised one, against Chen and Ballance's hand design.

**The problem.** `P(s) = k (s + a) / (s² − 2.5)`, `k ∈ [1, 10]`, `a ∈ [0.1, 1]`, nominal `(s+1)/(s²−2.5)`: one pole in the right half-plane at every member. Stability margin **2.1** and nothing else, at ω = 0.1, 1, 2, 6 and 50 rad/s.

**Published controller.** A static gain, **5.18**, searched in `(0, 10⁸]`; Chen and Ballance's is 6.582.

**What the published gain does.** With `C` a constant the closed loop's characteristic polynomial is `s² + kC s + (kC a − 2.5)`, stable if and only if `kC a > 2.5`. At `k = 1` that needs `C > 2.5/a`: **5.18 leaves every member with `a < 0.48` unstable**, and 6.582 every one with `a < 0.38`. No static gain below 25 stabilises the worst member, `k = 1, a = 0.1`. The margin at the five design frequencies does not see this - it is met by gains from about 20 - and the verifier, which checks the specifications and not the closed-loop poles of every member, reports gains from 20 as meeting everything. That gap, between a margin sampled at the design frequencies and the stability of every member, is why the controller here is also checked against the closed-loop poles of the family.

**How it is posed here.** The search with a static gain declares the box `[1, 10⁸]` infeasible although `[20, 10⁸]` in it is not, which is a defect under study; the file carries **one zero and one pole** in `[0.01, 1000]`, `k ∈ [0.01, 10⁸]`.

**What came out.** ε = 0.5: `0.0738 (s + 1000) / (s + 0.968)`, worst excess −0.008 dB - and **0 of the 81 members of the 9 × 9 family unstable**, by the roots of the closed-loop characteristic polynomial of each. A strong lag: low-frequency gain of about 76, high-frequency gain 0.074.

## What is not here, and why

Six problems of the table were posed and run and are not here, because a file
without a controller says the toolbox cannot solve the problem, and that is
better said in words:

- **`aircraft`, `aircraft-w9`, `flight22`** - the aircraft of Thompson and Nwokah at three sets of frequencies: the published structures (two zeros and two poles; four and four) ran six hours at every tolerance without finishing, and the smaller ones came back infeasible in seconds. With the plant's five uncertain parameters and 6 dB of margin the problem needs the five-parameter controller, and the branch-and-bound over intervals does not reach five parameters in that time.
- **`fopdt`** - Purohit's first-order-plus-delay plant: five structures, from one zero and one pole to two and three, none finished in six hours at any tolerance.
- **`maglev-lower`** - the stable half of the maglev: the paper's order and ours ran six hours without finishing, and one zero with two poles returns no controller. Its unstable half, `maglev-upper`, solves in seconds.
- **`jet-g12`** - Tharewal 6.2: as transcribed from the PDF the tracking corridor is narrower than the template below 0.06 rad/s and the problem is infeasible whatever the structure and whatever the margin - with the paper's own `γ = 1.001` and with `1.2`. The plant's two uncertain coefficients and the corridor are marked as doubtful readings in the corpus; until somebody checks them against the original the problem stays out.

The line these six draw is the dimension of the search: every problem here is found with four search parameters or fewer, and none of the five-parameter ones finished.

The corpus these come from has more problems than the toolbox can take. Left
out, with the reason:

- **fractional-order plants or controllers** (Tharewal 4.1, 4.2, 4.3; Nandkishor 5.4; Nataraj and Kalla 2010): the toolbox has no `s^μ`.
- **multi-input multi-output** (Patil and Nataraj 2012, the 2×2 maglev).
- **non-parametric uncertainty** (IFAC 2008 ex. 2; Deshpande and Nataraj 2015 ex. 2), where the family is a magnitude envelope and not a box of parameters.
- **H∞ criteria** (Tharewal 5.1), where two specifications are added rather than intersected.
- **fixed plants with a bandwidth specification** (IFAC 2008 ex. 1; DYCOPS 2013 §3.4; Deshpande and Nataraj 2015 ex. 1): no uncertainty, so no templates.
- **experimental setups** (Jeyasenthil and Nataraj 2015 ex. 2), whose uncertainty ranges are not published.
- **prefilter design** (Tharewal, chapter 7): the toolbox has no prefilter.
- **Tharewal 6.1**, where the paper's own answer is that no feasible solution exists - worth returning to as a test of the infeasibility verdict rather than as a design.

Two of the QFT Toolbox manual's examples are reproducible but not yet done:
**example 9**, the flexible Philips mechanism, whose uncertainty is in a
coefficient of the numerator and whose margin comes with weights, and
**example 10**, an inverted pendulum, which is unstable and has a delay. Of
the other ten, example 3 has non-parametric uncertainty, 4 a fixed plant, 6 a
missile in three cases with an unstable controller, 7, 8 and 15 are multiloop,
11 is an experimental response and 12 to 14 are discrete-time.
