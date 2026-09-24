# Design problems from the literature, solved with QFTbx

Each file here is a published QFT design problem: the plant and its
uncertainty, the specifications, the design frequencies and the structure of
the controller are the paper's, and the file carries everything QFTbx computed
from them - the templates and their contours, the boundaries, the controller
found and the verifier's verdict on it, the closed-loop stability of every plant
of the sweep included. Each file records the sweep its templates came from, so
the verdict can be recomputed from the file alone.

Open any of them with QFTbx to see every phase.

The plants are grouped by the family they come from. `k a/(s(s+a))` and
`k/(s(s+a))` are the same DC motor written two ways, and which of the two a
paper uses changes the templates - it is not a typo in one of them.

## The problems

| Problem | Source | Plant and uncertainty | Specifications | Design frequencies | Published controller | Controller in the file |
|---|---|---|---|---|---|---|
| `toolbox-1` | QFT Toolbox manual (Borghesani, Chait and Yaniv), example 1 | `k/((s+a)(s+b))`; `k ∈ [1,10]`, `a ∈ [1,5]`, `b ∈ [20,30]` | stability `1.2`; output disturbance `0.02(s³+64s²+748s+2400)/(s²+14.4s+169)` over [0,10]; input disturbance `0.01` over [0,50] | 0.1 5 10 100 | designed by hand in the manual | 1 zero, 1 pole, gain 67.6961 |
| `toolbox-2` | QFT Toolbox manual, example 2 - the problem most of the rest descend from | `k a/(s(s+a))`; `k,a ∈ [1,10]` | stability `1.2`; corridor `120/(s³+17s²+82s+120)` to `0.6584(s+30)/(s²+4s+19.752)` | 0.1 0.5 1 2 15 100 | designed by hand in the manual; second order by genetic algorithm in Chen, Ballance and Gawthrop 1998 (`k_hf` 136.76 dB); CRONE-2 in Cervera and Baños 2008 (`K_hf` 129.75 dB) | 1 zero, 1 pole, gain 567.318 |
| `acc90` | ACC'90 benchmark, spring-mass; QFT Toolbox manual example 5 (margin 2.25); Nataraj and Kubal, IJRNC 17 (2007), ex. 4.1 (p. 262-263) | `e/(s²(s²+0.02s+2e))`; `e ∈ [0.5,2]` — a double integrator | stability `1.75` and nothing else | 0.1 0.98 0.99 1 2 5 7 8.5 10 15 20 100 | Nataraj and Kubal, for their margin specifications: `1.139e7(s+0.0751)(s+0.3488)(s+0.3868)/((s+7.2019)(s+39.7899)(s+95.8659)(s+96.2539))` | 1 zero, 1 pole, gain 1000 |
| `dcm-k` | Tharewal 2005, ex. 3.1 (p. 37-38) = Nataraj and Tharewal, ASME 2007, ex. 5.1 | `k/(s(s+a))`, `k,a ∈ [1,10]` | stability `1.2`; tracking `T_U = 0.6584(s+30)/(s²+4s+19.752)`, `T_L = 120/(s³+17s²+82s+120)` | 0.1 0.5 1 15 100 | `3462219(s+3.85)/((s+931.27)(s+946.83))` — 1 zero, 2 poles | 1 zero, 2 poles, gain 49419.2 |
| `dcm-ka-w5` | Chait, Chen and Hollot, ASME JDSMC 121 (1999) | `k a/(s(s+a))`, `k,a ∈ [1,10]` | the same as `dcm-k` | 0.1 0.5 1 15 100 | three degrees of freedom by linear programming | 1 zero, 1 pole, gain 569.783 |
| `dcm-ka-w8` | Purohit, Goldsztejn, Jermann, Granvilliers, Goualard and Nataraj, IJRNC 2016, exp. 4.1 (p. 10-12) | `k a/(s(s+a))`, `k,a ∈ [1,10]` | stability `1.2`; the same corridor | 0.5 1 2 3 5 10 30 60 | PID `Kp 9.22, Td 0.41, Ti 12.21` and a prefilter; **the paper admits it violates the upper tracking bound over ω ∈ [11,29]** | 2 zeros, 1 pole, gain 0.333575 |
| `dcm-AC` | IFAC DYCOPS 2013 (p. 431-432); Tharewal 2005, ex. 3.2 | `k a/(s(s+a))`, `k,a ∈ [1,10]` | stability `1.2`; tracking with the fourth-order lower bound `8400/((s+3)(s+4)(s+10)(s+70))` | 0.5 1 2 10 30 60 | PID `7.03 + 3.89s + 0.1/s` | 2 zeros, 1 pole, gain 0.311709 |
| `dcm-T33` | Tharewal 2005, ex. 3.3 (p. 40-41) = ASME 2007, ex. 5.3 | `k/(s(s+a))`, `k,a ∈ [1,10]` | stability `1.2`; tracking `T_U = 1.5/(s+1.5)`, `T_L = 1/(s+1)²` | 0.001 0.0157 0.2449 3.8337 60 | `10455(s+1.56)(s+1.29)/((s+0.54)(s²+149.4s+17260))` — **a complex pole pair** | 1 zero, 1 pole, gain 89.3251 |
| `dcm-hs72` | Bryant and Halikias 1995, over Horowitz and Sidi 1972 | `k a/(s(s+a))`, `k,a ∈ [1,10]` | the corridor `1/(s+1)² ≤ T ≤ 1.5/(s+1.5)`; the paper states no margin, `1.2` added here as Tharewal 3.3 does | 23 logarithmic, 0.01 to 428.1 | by linear programming | 1 zero, 2 poles, gain 27395.5 |
| `msf` | Tharewal 2005, ex. 4.4 (p. 68-72) | MSF desalination `K(1+T1 s)/((1+T2 s)(1+T3 s))`; `K ∈ [32,76]`, `T1 ∈ [12,28]`, `T2 ∈ [11,26]`, `T3 ∈ [4,10]` | stability `1.2`; tracking added by the authors ("Ismail does not use any tracking specifications") | 0.01 0.098 0.309 0.97 9.558 30 | `655.65(s+2.022)/(s(s+169.9))` — **an integrator** | 1 zero, 2 poles, gain 245.613 |
| `maglev-lower` | Purohit, Goldsztejn, Jermann, Granvilliers, Goualard and Nataraj, IJRNC 2016, exp. 4.2 (p. 12-13) | `k/(s²+a)`; `k ∈ [811,944]`, `a ∈ [382,478.5]` — poles on the imaginary axis | stability `1.2`; corridor `916.3/(s³+39.76s²+354.9s+916.3)` to `(1.722s+68.89)/(s²+16.6s+68.89)` | 11 from 0.1 to 30 | PID with `ωn`, `ζ` | 2 zeros, 1 pole, gain 0.01 |
| `maglev-upper` | the same paper, the unstable half | `k/(s²−a)`; `k ∈ [1021,1106]`, `a ∈ [382,478.5]` | the same | the same | PID with `ωn`, `ζ` | 1 zero, 1 pole, gain 715.092 |
| `fopdt` | the same paper, exp. 4.3 (p. 15-16) | first order plus delay, first-order Padé: `k(1−td s/2)/((s+a)(1+td s/2))`; `k ∈ [1,3]`, `a ∈ [1,2]`, `td ∈ [0.08,0.12]` | stability `1.2`; corridor `9/(s³+7s²+15s+9)` to `4/(s²+3.3s+4)` | 0.1 0.2 0.5 1 2 5 8 10 50 | PID `Kp 1.88, Td 0.05, Ti 0.72` | 2 zeros, 1 pole, gain 0.0781889 |
| `unstable` | Tharewal 2005, ex. 3.6 (p. 47-48) | unstable `k(s+a)/(s²−2.5)`; `k ∈ [1,10]`, `a ∈ [0.1,1]` | stability `2.1` and nothing else ("the only design spec considered") | 0.1 1 2 6 50 | `5.18` — a static gain, searched in `(0,10⁸]`; Chen and Ballance's hand design, `6.582` | 1 zero, 1 pole, gain 0.0738173 |

## The problems, one by one

### toolbox-1 — example 1 of the QFT Toolbox manual

**Source.** Borghesani, Chait and Yaniv, the QFT Frequency Domain Control Design Toolbox manual, example 1 - the one the manual walks through first.

**The problem.** `P(s) = k / ((s + a)(s + b))`, `k ∈ [1, 10]`, `a ∈ [1, 5]`, `b ∈ [20, 30]`. Stability 1.2 over [0.1, 100]; output disturbance `|1/(1+L)| ≤ 0.02 (s³+64s²+748s+2400)/(s²+14.4s+169)` over [0.1, 10]; input disturbance `|P/(1+L)| ≤ 0.01` over [0.1, 50]; ω = 0.1, 5, 10, 100 rad/s.

**Published controller.** Designed by hand in the manual's loop-shaping environment.

**The controller in the file.** 1 zero, 1 pole, gain **67.6961**, worst excess over the specifications **-0.8504 dB**, and every one of the 729 plants of the sweep closed-loop stable.

**Why the gain is not the smallest that meets the bounds.** A gain of 20.6304 meets every bound at the four design frequencies and leaves **435 of the 729 plants closed-loop unstable**: the crossing that decides stability falls between two design frequencies. The search now closes the loop with every plant of the sweep before it returns a design, so it climbs to 67.6961, which is what stabilising the whole family costs here.

### toolbox-2 — example 2 of the QFT Toolbox manual

**Source.** Borghesani, Chait and Yaniv, the QFT Toolbox manual, example 2. Half the DC motors of this battery descend from it, at other frequencies or with other bounds; this is the original.

**The problem.** `P(s) = k a / (s (s + a))`, `k, a ∈ [1, 10]`; stability 1.2; tracking between `120/(s³+17s²+82s+120)` and `0.6584 (s+30)/(s²+4s+19.752)`; ω = 0.1, 0.5, 1, 2, 15, 100 rad/s.

**Published controllers.** The manual's, by hand. Chen, Ballance and Gawthrop 1998, second order by genetic algorithm, `k_hf` 136.76 dB. Cervera and Baños 2008, CRONE-2 with `n_pe = 3`, `K_hf` 129.75 dB. Chen et al. print the coefficient as 0.6854 and "828" for 82 - readings of the manual's 0.6584 and 82.

**The controller in the file.** 1 zero, 1 pole, gain **567.318**, worst excess over the specifications **-0.0001 dB**.

### acc90 — the ACC'90 spring-mass benchmark

**Source.** The ACC'90 benchmark problem; example 5 of the QFT Toolbox manual (Borghesani, Chait and Yaniv), where it is designed with a margin of 2.25; Nataraj and Kubal, IJRNC 17 (2007), ex. 4.1, p. 262-263, with margin specifications of their own.

**The problem.** `P(s) = e / (s² (s² + 0.02 s + 2e))`, `e ∈ [0.5, 2]`: two masses joined by an uncertain spring, a double integrator, a lightly damped pair at `√(2e)`. Stability margin **1.75** and nothing else, at ω = 0.1, 0.98, 0.99, 1, 2, 5, 7, 8.5, 10, 15, 20 and 100 rad/s. The three frequencies around 1 rad/s straddle the resonance, which sweeps from 1 to 2 rad/s across the family.

**Published controller.** Nataraj and Kubal, for their margins: `1.139·10⁷ (s+0.0751)(s+0.3488)(s+0.3868) / ((s+7.2019)(s+39.7899)(s+95.8659)(s+96.2539))`, three zeros and four poles.

**Why this structure.** Nataraj and Kubal's controller answers their own margin specifications, not the single stability margin posed here, so it is not the controller of this problem; the file holds one zero and one pole.

**The controller in the file.** 1 zero, 1 pole, gain **1000**, worst excess over the specifications **-4.8477 dB**.

### dcm-k — the DC motor of Tharewal 3.1

**Source.** Tharewal 2005, example 3.1 (p. 37-38), the same as Nataraj and Tharewal, ASME JDSMC 2007, example 5.1; the design Chen, Ballance and Gawthrop 1998 made with a genetic algorithm is what it is compared against there.

**The problem.** `P(s) = k / (s (s + a))`, `k, a ∈ [1, 10]` - note **`k`, not `k·a`** in the numerator: this is Tharewal's plant, and its templates are not those of the QFT Toolbox's example 2 although the specifications are. Stability margin 1.2; tracking between `T_L = 120/(s³+17s²+82s+120)` and `T_U = 0.6584 (s+30)/(s²+4s+19.752)`, at ω = 0.1, 0.5, 1, 15 and 100 rad/s.

**Published controller.** `3462219 (s+3.85) / ((s+931.27)(s+946.83))`: one zero, two poles, a high-frequency gain of 3.46·10⁶, "a 48.73 % reduction" against Chen et al.

**The controller in the file.** 1 zero, 2 poles, gain **49419.2**, worst excess over the specifications **-0.0005 dB**.

**How to read it.** Both controllers are zero-pole-gain forms with one more pole than zero, so the gain of each is its high-frequency gain and the two numbers are comparable in kind: 4.9·10⁴ here against 3.5·10⁶ published, with the reservation that the paper's boundaries come from its own templates and tolerance. The tracking bound's coefficient is the correct **0.6584**; several papers carry 0.6854, a transposition that travels from one to the next.

### dcm-ka-w5 — the QFT Toolbox motor at Chait's frequencies

**Source.** Chait, Chen and Hollot, "Automatic loop-shaping of QFT controllers via linear programming", ASME JDSMC 121 (1999): the QFT Toolbox's example 2 at five design frequencies instead of six.

**The problem.** `P(s) = k a / (s (s + a))`, `k, a ∈ [1, 10]`; stability 1.2; tracking between `120/(s³+17s²+82s+120)` and `0.6584 (s+30)/(s²+4s+19.752)`; ω = 0.1, 0.5, 1, 15, 100 rad/s. Against `toolbox-2` the only change is the missing 2 rad/s.

**Published controller.** A controller of three degrees of freedom found by linear programming; the paper writes the tracking coefficient as 0.6854, a transposition of the Toolbox manual's 0.6584 - the one used here.

**The controller in the file.** 1 zero, 1 pole, gain **569.783**, worst excess over the specifications **-0.0003 dB**.

**How to read it.** Next to `toolbox-2` (gain 567.32 with the sixth frequency): dropping 2 rad/s costs nothing here, the binding frequencies being elsewhere.

### dcm-ka-w8 — the QFT Toolbox motor at Purohit's eight frequencies

**Source.** Purohit, Goldsztejn, Jermann, Granvilliers, Goualard and Nataraj, IJRNC 2016, experiment 4.1 (p. 10-12).

**The problem.** `P(s) = k a / (s (s + a))`, `k, a ∈ [1, 10]`, which the paper samples at 5 × 5 = 25 plants; stability 1.2; tracking between `120/(s³+17s²+82s+120)` and `0.6585 (s+30)/(s²+4s+19.752)`; ω = 0.5, 1, 2, 3, 5, 10, 30, 60 rad/s.

**Published controller.** A PID, `Kp = 9.22`, `Td = 0.41`, `Ti = 12.21`, with a prefilter. **The paper itself records that it violates the upper tracking bound over ω ∈ [11, 29] rad/s.**

**Why this structure.** The published controller is a PID, which has an integrator; QFTbx's controllers are real zeros, real poles and a gain, with no integrator, so the file holds the PID's two zeros over one real pole instead.

**The controller in the file.** 2 zeros, 1 pole, gain **0.333575**, worst excess over the specifications **-0.0078 dB**.

**How to read it.** For a two-zero, one-pole controller the gain is the coefficient of `s` at high frequency, the counterpart of the PID's `Kd = Kp·Td = 3.78`.

### dcm-AC — the DC motor with the fourth-order lower bound

**Source.** IFAC DYCOPS 2013 (p. 431-432), the same problem as Tharewal 2005 example 3.2 (p. 38-39), against the optimal PID of Zolotas and Halikias.

**The problem.** `P(s) = k a / (s (s + a))`, `k, a ∈ [1, 10]`; stability 1.2; tracking between the fourth-order `8400/((s+3)(s+4)(s+10)(s+70))` and `0.6584 (s+30)/(s²+4s+19.752)` - the DYCOPS paper prints the two labels the other way round; ω = 0.5, 1, 2, 10, 30, 60 rad/s.

**Published controller.** The PID `7.03 + 3.89 s + 0.1/s`; Tharewal's is `12.1 + 3.53 s + 0.38/s`.

**Why this structure.** The published controllers are PIDs; QFTbx's controllers have no integrator, so the file holds two zeros over one real pole.

**The controller in the file.** 2 zeros, 1 pole, gain **0.311709**, worst excess over the specifications **-0.0060 dB**.

**How to read it.** The gain is the coefficient of `s` at high frequency, against the PID's `Kd = 3.89`.

### dcm-T33 — Tharewal 3.3, first-order bounds at very low frequencies

**Source.** Tharewal 2005, example 3.3 (p. 40-41) = ASME JDSMC 2007, example 5.3, against the linear-programming design of Bryant and Halikias.

**The problem.** `P(s) = k / (s (s + a))`, `k, a ∈ [1, 10]`; stability 1.2; tracking between `1/(s+1)²` and `1.5/(s+1.5)`; ω = 0.001, 0.0157, 0.2449, 3.8337 and 60 rad/s - three decades below 1 rad/s, where the bounds are nearly flat.

**Published controller.** `10455 (s+1.56)(s+1.29) / ((s+0.54)(s²+149.4s+17260))`, two zeros, one real pole and a complex pair, searched in a 30 % neighbourhood of Bryant and Halikias's parameters.

**Why this structure.** The published controller has a complex pole pair, which QFTbx's controllers - real zeros, real poles and a gain - cannot hold, so it cannot be posed as published; the file holds one zero and one pole.

**The controller in the file.** 1 zero, 1 pole, gain **89.3251**, worst excess over the specifications **-5.5762 dB**, and every one of the 625 plants of the sweep closed-loop stable.

**Why the gain is not the smallest that meets the bounds.** A gain of 37.4566 meets every bound at the five design frequencies and leaves **286 of the 625 plants closed-loop unstable**. As in `toolbox-1`, the design that stabilises the whole family costs more gain.

**How to read it.** The gain is a high-frequency gain, against the paper's 10 455 - a different structure, so a comparison of kind and not of merit.

### dcm-hs72 — Horowitz and Sidi's corridor, by Bryant and Halikias

**Source.** Bryant and Halikias 1995, over the problem of Horowitz and Sidi 1972; Tharewal 2005 example 3.3 solves the same bounds on the plant `k/(s(s+a))` at five frequencies (`dcm-T33`).

**The problem.** `P(s) = k a / (s (s + a))`, `k, a ∈ [1, 10]`; the corridor `1/(s+1)² ≤ T ≤ 1.5/(s+1.5)`; 23 frequencies spaced logarithmically from 0.01 to 428.1 rad/s. The paper states no stability margin; **1.2 is added here**, as Tharewal does in 3.3.

**Published controller.** `13721 (s+1.21)(s+1) / ((s+0.599)(s²+150s+150²))`: two zeros, one real pole and a complex pair, found by linear programming.

**Why this structure.** The published controller has a complex pole pair, which QFTbx's controllers cannot hold; the file holds one zero and two poles.

**The controller in the file.** 1 zero, 2 poles, gain **27395.5**, worst excess over the specifications **-0.0003 dB**.

**How to read it.** The gain, one more pole than zero, is a high-frequency gain, against the paper's 13 721 - a different structure, so a comparison of kind and not of merit.

### msf — the MSF desalination plant of Tharewal 4.4

**Source.** Tharewal 2005, example 4.4 (p. 68-72): the top-brine-temperature loop of a multi-stage flash desalination plant, over the model of Ismail. Tharewal uses it for an integer-order against a fractional-order controller.

**The problem.** `P(s) = K (1 + T1 s) / ((1 + T2 s)(1 + T3 s))`, four uncertain parameters: `K ∈ [32, 76]`, `T1 ∈ [12, 28]`, `T2 ∈ [11, 26]`, `T3 ∈ [4, 10]`; nominal `54 (1 + 20.32 s)/((1 + 18.3 s)(1 + 7.2 s))`. Stability margin 1.2; tracking between `60.44/(s³ + 13.5 s² + 50.94 s + 60.44)` and `(16.32 s + 197.9)/(12.12 s² + 64.65 s + 197.9)`, which the authors add themselves ("Ismail does not use any tracking specifications"); ω = 0.01, 0.098, 0.309, 0.97, 9.558, 30 rad/s. A stable plant with no integrator and slow time constants.

**Published controller.** `655.65 (s + 2.022) / (s (s + 169.9))`: one zero, an integrator and one pole, searched in `{[1, 10⁸], [0.001, 50], [0.001, 1500], [0.001, 1500]}`.

**Why this structure.** The published controller has an integrator, which QFTbx's controllers do not; the file holds the same zero over two real poles.

**The controller in the file.** 1 zero, 2 poles, gain **245.613**, worst excess over the specifications **-0.0000 dB**.

**How to read it.** With one more pole than zero the gain is a high-frequency gain, against the paper's 655.65 with its integrator. The integrator is what gives the published design zero steady-state error; the design here does not have it, and reads as a frequency-domain comparison only.

### maglev-lower — Purohit's maglev on the stable side of its equilibrium

**Source.** Purohit, Goldsztejn, Jermann, Granvilliers, Goualard and Nataraj, IJRNC 2016, experiment 4.2 (p. 12-13): a magnetic levitation system linearised on either side of its equilibrium. This is the half whose poles are on the imaginary axis; `maglev-upper` is the unstable one.

**The problem.** `P(s) = k / (s² + a)`, `k ∈ [811, 944]`, `a ∈ [382, 478.5]`: a pair of poles on the imaginary axis at `±j√a ≈ ±20 rad/s` at every member, so no member is asymptotically stable on its own. Stability margin 1.2; tracking between `916.3/(s³ + 39.76 s² + 354.9 s + 916.3)` and `(1.722 s + 68.89)/(s² + 16.6 s + 68.89)`; eleven frequencies from 0.1 to 30 rad/s.

**Published controller.** A PID with a second-order filter, the same family as for the unstable half.

**Why this structure.** A PID with a filter has an integrator and a complex pole pair, and QFTbx's controllers hold neither; the file holds two zeros and one pole.

**The controller in the file.** 2 zeros, 1 pole, gain **0.01**, worst excess over the specifications **-0.0009 dB**.

**How to read it.** The gain is the floor of the search box, `[0.01, 10⁸]`: the specifications are met by any gain that small, so the number is the smallest the box allows and not the smallest the problem allows. The plant already carries a gain above 800, which is where the loop's magnitude comes from. As with `unstable`, the controller is also checked against the closed-loop poles of the family, since a margin sampled at the design frequencies does not see what a pole on the imaginary axis does: over an 11 × 11 grid of `k` and `a` the worst closed-loop pole is at **-0.649**, so every member is stable.

### maglev-upper — the unstable half of Purohit's maglev

**Source.** Purohit, Goldsztejn, Jermann, Granvilliers, Goualard and Nataraj, IJRNC 2016, experiment 4.2 (p. 12-13): a magnetic levitation system linearised on either side of its equilibrium.

**The problem.** `P(s) = k / (s² − a)`, `k ∈ [1021, 1106]`, `a ∈ [382, 478.5]`: one real pole in the right half-plane at every member, at `√a ≈ 20 rad/s`. Stability margin 1.2; tracking between `916.3/(s³ + 39.76 s² + 354.9 s + 916.3)` and `(1.722 s + 68.89)/(s² + 16.6 s + 68.89)`; eleven frequencies from 0.1 to 30 rad/s.

**Published controller.** A PID with a second-order filter, `Kp = 2.956`, `Td = 0.0604`, `Ti = 0.4195`, `ωn = 5.0075`, `ζ = 0.995`.

**Why this structure.** A PID with a second-order filter has an integrator and a complex pole pair, neither of which QFTbx's controllers hold; the file holds one zero and one pole.

**The controller in the file.** 1 zero, 1 pole, gain **715.092**, worst excess over the specifications **-0.0000 dB**.

**How to read it.** A gain of 715 with one zero and one pole is a lead: the zero below the pole, lifting the phase around the crossover the unstable pole forces above 20 rad/s.

### fopdt — first order plus delay, Purohit 4.3

**Source.** Purohit et al., IJRNC 2016, experiment 4.3 (p. 15-16).

**The problem.** A first-order plant with a delay, the delay written as a first-order Padé: `P(s) = k (1 − td s/2) / ((s + a)(1 + td s/2))`, `k ∈ [1, 3]`, `a ∈ [1, 2]`, `td ∈ [0.08, 0.12]`. The Padé puts a zero in the right half-plane at `2/td`, between 16.7 and 25 rad/s, which is what limits the bandwidth. Stability margin 1.2; tracking between `9/(s³ + 7 s² + 15 s + 9)` and `4/(s² + 3.3 s + 4)`; ω = 0.1, 0.2, 0.5, 1, 2, 5, 8, 10 and 50 rad/s.

**Published controller.** A PID, `Kp = 1.88`, `Td = 0.05`, `Ti = 0.72`.

**Why this structure.** A PID has an integrator, which QFTbx's controllers do not hold; the file holds two zeros and one pole.

**The controller in the file.** 2 zeros, 1 pole, gain **0.0781889**, worst excess over the specifications **-0.0024 dB**.

**How to read it.** Three uncertain parameters, so the template is a volume sampled at nine points on each: the problem that costs the most template of the fourteen. The zero in the right half-plane cannot be cancelled, so the loop has to be shaped below it; the two zeros of the controller, at 2.33 and 25.03, sit on either side of it.

### unstable — Tharewal's unstable plant, stability alone

**Source.** Tharewal 2005, example 3.6 (p. 47-48), designed directly on the unstable nominal plant rather than on a stabilised one, against Chen and Ballance's hand design.

**The problem.** `P(s) = k (s + a) / (s² − 2.5)`, `k ∈ [1, 10]`, `a ∈ [0.1, 1]`, nominal `(s+1)/(s²−2.5)`: one pole in the right half-plane at every member. Stability margin **2.1** and nothing else, at ω = 0.1, 1, 2, 6 and 50 rad/s.

**Published controller.** A static gain, **5.18**, searched in `(0, 10⁸]`; Chen and Ballance's is 6.582.

**Why this structure.** The published controller is a static gain, and no static gain stabilises the whole family (below); the file holds one zero and one pole, which does.

**What the published gain does.** With `C` a constant the closed loop's characteristic polynomial is `s² + kC s + (kC a − 2.5)`, stable if and only if `kC a > 2.5`. At `k = 1` that needs `C > 2.5/a`: **5.18 leaves every member with `a < 0.48` unstable**, and 6.582 every one with `a < 0.38`. No static gain below 25 stabilises the worst member, `k = 1, a = 0.1`. The margin at the five design frequencies does not see this - it is met by gains from about 20 - and the verifier, which checks the specifications and not the closed-loop poles of every member, reports gains from 20 as meeting everything. That gap, between a margin sampled at the design frequencies and the stability of every member, is why the controller here is also checked against the closed-loop poles of the family.

**The controller in the file.** 1 zero, 1 pole, gain **0.0738173**, worst excess over the specifications **-0.0080 dB**.
