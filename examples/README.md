# Design problems from the literature, solved with QFTbx

Each file here is a published QFT design problem: the plant and its
uncertainty, the specifications, the design frequencies and the structure of
the controller are the paper's, except where a page below says otherwise, and
the file carries everything QFTbx computed from them - the templates and their
contours, the boundaries, the controller found and the verifier's verdict on
it, the closed-loop stability of every plant of the sweep included. Each file records the sweep its templates came from, so
the verdict can be recomputed from the file alone.

Open any of them with QFTbx to see every phase.

The plants are grouped by the family they come from. `k a/(s(s+a))` and
`k/(s(s+a))` are the same DC motor written two ways, and which of the two a
paper uses changes the templates - it is not a typo in one of them.

## How the controllers were found

Every controller here is MC2's, at a tolerance of 0.5 and reading every point of
the templates exactly. It meets every specification at every point of the
templates at the design frequencies, it closes a stable loop with every plant of
the sweep, and its gain is the least that the controller's structure admits in
the search box, to within the tolerance. Unless that gain is the floor of the
box, a design found that way rests on the bound that holds it, so its worst
excess over the specifications is zero to within 10⁻¹² dB.

## Two versions of a problem

A specification asked at the design frequencies is met at them, and a search
that minimises the gain finds the designs that meet it there and nowhere else.
`dcm-T33`, as Tharewal poses it, meets its stability margin at its five
frequencies and exceeds it by 43 dB at 28 rad/s, where a lightly damped
closed-loop pair resonates. So every problem whose design exceeds its stability
margin at some frequency comes in two files:

- `name.qft` is the problem as the paper poses it, at the paper's design
  frequencies and with the margin over the paper's range;
- `name-extended.qft` asks the stability margin at every frequency, from a
  hundredth of the lowest design frequency to 10⁶ rad/s. Over a grid of 1001
  logarithmically spaced frequencies, the one where the design exceeds the
  margin most is added to the design frequencies and the controller searched
  again, until no frequency of the grid exceeds it.

The extended version changes the stability margin and nothing else: the other
specifications keep the paper's frequencies and ranges, and between those
frequencies a tracking bound can still be exceeded by some tenths of a dB, as in
any design over a finite set of frequencies. The description inside each file
says which of the two it is. `dcm-hs72` meets its margin at every frequency as
posed, and has one file; `toolbox-1` has one file too, for the reason its page
gives.

## The problems

| Problem | Source | Plant and uncertainty | Specifications | Design frequencies | Published controller | Controller in the file |
|---|---|---|---|---|---|---|
| `toolbox-1` | QFT Toolbox manual (Borghesani, Chait and Yaniv), example 1 | `k/((s+a)(s+b))`; `k ∈ [1,10]`, `a ∈ [1,5]`, `b ∈ [20,30]` | stability `1.2`; output disturbance `0.02(s³+64s²+748s+2400)/(s²+14.4s+169)` over [0,10]; input disturbance `0.01` over [0,50] | 0.1 5 10 100 | designed by hand in the manual | 1 zero, 1 pole, gain 50.2204 |
| `toolbox-2` | QFT Toolbox manual, example 2 - the problem most of the rest descend from | `k a/(s(s+a))`; `k,a ∈ [1,10]` | stability `1.2`; corridor `120/(s³+17s²+82s+120)` to `0.6584(s+30)/(s²+4s+19.752)` | 0.1 0.5 1 2 15 100 | designed by hand in the manual; second order by genetic algorithm in Chen, Ballance and Gawthrop 1998 (`k_hf` 136.76 dB); CRONE-2 in Cervera and Baños 2008 (`K_hf` 129.75 dB) | 1 zero, 1 pole, gain 568.394 |
| `acc90` | ACC'90 benchmark, spring-mass; QFT Toolbox manual example 5 (margin 2.25); Nataraj and Kubal, IJRNC 17 (2007), ex. 4.1 (p. 262-263) | `e/(s²(s²+0.02s+2e))`; `e ∈ [0.5,2]` — a double integrator | stability `1.75` and nothing else | 0.1 0.98 0.99 1 2 5 7 8.5 10 15 20 100 | Nataraj and Kubal, for their margin specifications: `1.139e7(s+0.0751)(s+0.3488)(s+0.3868)/((s+7.2019)(s+39.7899)(s+95.8659)(s+96.2539))` | 1 zero, 2 poles, gain 1000 |
| `dcm-k` | Tharewal 2005, ex. 3.1 (p. 37-38) = Nataraj and Tharewal, ASME 2007, ex. 5.1 | `k/(s(s+a))`, `k,a ∈ [1,10]` | stability `1.2`; tracking `T_U = 0.6584(s+30)/(s²+4s+19.752)`, `T_L = 120/(s³+17s²+82s+120)` | 0.1 0.5 1 15 100 | `3462219(s+3.85)/((s+931.27)(s+946.83))` — 1 zero, 2 poles | 1 zero, 2 poles, gain 42527.4 |
| `dcm-ka-w5` | Chait, Chen and Hollot, ASME JDSMC 121 (1999) | `k a/(s(s+a))`, `k,a ∈ [1,10]` | the same as `dcm-k` | 0.1 0.5 1 15 100 | three degrees of freedom by linear programming | 1 zero, 1 pole, gain 568.394 |
| `dcm-ka-w8` | Purohit, Goldsztejn, Jermann, Granvilliers, Goualard and Nataraj, IJRNC 2016, exp. 4.1 (p. 10-12) | `k a/(s(s+a))`, `k,a ∈ [1,10]` | stability `1.2`; the same corridor | 0.5 1 2 3 5 10 30 60 | PID `Kp 9.22, Td 0.41, Ti 12.21` and a prefilter; **the paper admits it violates the upper tracking bound over ω ∈ [11,29]** | 2 zeros, 1 pole, gain 0.326047 |
| `dcm-AC` | IFAC DYCOPS 2013 (p. 431-432); Tharewal 2005, ex. 3.2 | `k a/(s(s+a))`, `k,a ∈ [1,10]` | stability `1.2`; tracking with the fourth-order lower bound `8400/((s+3)(s+4)(s+10)(s+70))` | 0.5 1 2 10 30 60 | PID `7.03 + 3.89s + 0.1/s` | 2 zeros, 1 pole, gain 0.300228 |
| `dcm-T33` | Tharewal 2005, ex. 3.3 (p. 40-41) = ASME 2007, ex. 5.3 | `k/(s(s+a))`, `k,a ∈ [1,10]` | stability `1.2`; tracking `T_U = 1.5/(s+1.5)`, `T_L = 1/(s+1)²` | 0.001 0.0157 0.2449 3.8337 60 | `10455(s+1.56)(s+1.29)/((s+0.54)(s²+149.4s+17260))` — **a complex pole pair** | 1 zero, 1 pole, gain 41.1014 |
| `dcm-hs72` | Bryant and Halikias 1995, over Horowitz and Sidi 1972 | `k a/(s(s+a))`, `k,a ∈ [1,10]` | the corridor `1/(s+1)² ≤ T ≤ 1.5/(s+1.5)`; the paper states no margin, `1.2` added here as Tharewal 3.3 does | 23 logarithmic, 0.01 to 428.1 | by linear programming | 1 zero, 2 poles, gain 26416.1 |
| `msf` | Tharewal 2005, ex. 4.4 (p. 68-72) | MSF desalination `K(1+T1 s)/((1+T2 s)(1+T3 s))`; `K ∈ [32,76]`, `T1 ∈ [12,28]`, `T2 ∈ [11,26]`, `T3 ∈ [4,10]` | stability `1.2`; tracking added by the authors ("Ismail does not use any tracking specifications") | 0.01 0.098 0.309 0.97 9.558 30 | `655.65(s+2.022)/(s(s+169.9))` — **an integrator** | 1 zero, 2 poles, gain 233.738 |
| `maglev-lower` | Purohit, Goldsztejn, Jermann, Granvilliers, Goualard and Nataraj, IJRNC 2016, exp. 4.2 (p. 12-13) | `k/(s²+a)`; `k ∈ [811,944]`, `a ∈ [382,478.5]` — poles on the imaginary axis | stability `1.2`; corridor `916.3/(s³+39.76s²+354.9s+916.3)` to `(1.722s+68.89)/(s²+16.6s+68.89)` | 11 from 0.1 to 30 | PID with `ωn`, `ζ` | 2 zeros, 1 pole, gain 0.01 |
| `maglev-upper` | the same paper, the unstable half | `k/(s²−a)`; `k ∈ [1021,1106]`, `a ∈ [382,478.5]` | the same | the same | PID with `ωn`, `ζ` | 1 zero, 1 pole, gain 708.503 |
| `fopdt` | the same paper, exp. 4.3 (p. 15-16) | first order plus delay, first-order Padé: `k(1−td s/2)/((s+a)(1+td s/2))`; `k ∈ [1,3]`, `a ∈ [1,2]`, `td ∈ [0.08,0.12]` | stability `1.2`; corridor `9/(s³+7s²+15s+9)` to `4/(s²+3.3s+4)` | 0.1 0.2 0.5 1 2 5 8 10 50 | PID `Kp 1.88, Td 0.05, Ti 0.72` | 2 zeros, 1 pole, gain 0.0639392 |
| `unstable` | Tharewal 2005, ex. 3.6 (p. 47-48) | unstable `k(s+a)/(s²−2.5)`; `k ∈ [1,10]`, `a ∈ [0.1,1]` | stability `2.1` and nothing else ("the only design spec considered") | 0.1 1 2 6 50 | `5.18` — a static gain, searched in `(0,10⁸]`; Chen and Ballance's hand design, `6.582` | 1 zero, 1 pole, gain 0.0729821 |

## The extended versions

The margin as posed is the worst excess over the stability margin on the grid of
1001 frequencies, with the frequency it happens at.

| Problem | Margin as posed | Frequencies added (rad/s) | Gain as posed | Gain extended |
|---|---|---|---|---|
| `toolbox-1` | +35.7 dB at 57.7 | none found yet: see its page | 50.2204 | - |
| `toolbox-2` | +2.9 dB at 217 | 2.917 3.516 5.433 217.3 267.3 | 568.394 | 1297.91 |
| `acc90` | +19.7 dB at 0.0332 | 0.004266 0.03319 | 1000 | 1000 |
| `dcm-k` | +8.0 dB at 54.2 | 4.236 4.325 54.2 62.66 63.97 | 42527.4 | 236142 |
| `dcm-ka-w5` | +2.9 dB at 217 | 2.917 3.516 5.433 217.3 267.3 272.9 | 568.394 | 1298.93 |
| `dcm-ka-w8` | +2.7 dB at 164 | 164 198.5 | 0.326047 | 0.680363 |
| `dcm-AC` | +2.6 dB at 155 | 154.8 183.9 187.4 | 0.300228 | 0.596137 |
| `dcm-T33` | +43.2 dB at 27.9 | 2.4547 2.455 2.518 21.68 27.93 | 41.1014 | 115.754 |
| `dcm-hs72` | met, by 1.2 dB | none: one file | 26416.1 | - |
| `msf` | +3.8 dB at 95.5 | 3.802 4.467 95.5 173.8 195 | 233.738 | 1484.59 |
| `maglev-lower` | +39.6 dB at 92.9 | 36.56 43.15 58.88 92.9 208.4 | 0.01 | 0.0821922 |
| `maglev-upper` | +72.7 dB at 853 | 13.24 15 15.63 89.13 100.9 107.4, with two zeros and two poles | 708.503 | 38.4827 |
| `fopdt` | +0.0008 dB at 4.90 | 4.898 | 0.0639392 | 0.0647 |
| `unstable` | +32.1 dB at 0.001 | 0.001 3.17 4.236 11.22 20.89 90.99 107.4 140.6 149.6 | 0.0729821 | 2.65673 |

## The problems, one by one

### toolbox-1 — example 1 of the QFT Toolbox manual

**Source.** Borghesani, Chait and Yaniv, the QFT Frequency Domain Control Design Toolbox manual, example 1 - the one the manual walks through first.

**The problem.** `P(s) = k / ((s + a)(s + b))`, `k ∈ [1, 10]`, `a ∈ [1, 5]`, `b ∈ [20, 30]`. Stability 1.2 over [0.1, 100]; output disturbance `|1/(1+L)| ≤ 0.02 (s³+64s²+748s+2400)/(s²+14.4s+169)` over [0.1, 10]; input disturbance `|P/(1+L)| ≤ 0.01` over [0.1, 50]; ω = 0.1, 5, 10, 100 rad/s.

**Published controller.** Designed by hand in the manual's loop-shaping environment.

**The controller in the file.** 1 zero, 1 pole, gain **50.2204**, and every one of the 729 plants of the sweep closed-loop stable. The zero sits at 1000, the top of the search box, over a pole at 132.8.

**Between the design frequencies.** The stability margin is exceeded by **35.7 dB** at 57.7 rad/s, between the design frequencies 10 and 100, where a lightly damped closed-loop pair resonates.

**No extended version yet.** With 57.7 rad/s added, MC2 proves that no controller with a single pole and no zero exists in the search box. With one zero and one pole it fills 8 GB of memory in half an hour without finishing, with two zeros and two poles it does not finish within an hour, and with one zero and two poles it ends without a design and without a proof that none exists. The file is the problem as the manual poses it, and has no `-extended` counterpart.

### toolbox-2 — example 2 of the QFT Toolbox manual

**Source.** Borghesani, Chait and Yaniv, the QFT Toolbox manual, example 2. Half the DC motors of this battery descend from it, at other frequencies or with other bounds; this is the original.

**The problem.** `P(s) = k a / (s (s + a))`, `k, a ∈ [1, 10]`; stability 1.2; tracking between `120/(s³+17s²+82s+120)` and `0.6584 (s+30)/(s²+4s+19.752)`; ω = 0.1, 0.5, 1, 2, 15, 100 rad/s.

**Published controllers.** The manual's, by hand. Chen, Ballance and Gawthrop 1998, second order by genetic algorithm, `k_hf` 136.76 dB. Cervera and Baños 2008, CRONE-2 with `n_pe = 3`, `K_hf` 129.75 dB. Chen et al. print the coefficient as 0.6854 and "828" for 82 - readings of the manual's 0.6584 and 82.

**The controller in the file.** 1 zero, 1 pole, gain **568.394**.

**Above the design frequencies.** The margin holds up to the last design frequency, 100 rad/s, and is exceeded by **2.9 dB** at 217 rad/s.

**The extended version.** `toolbox-2-extended.qft` adds 2.917, 3.516, 5.433, 217.3 and 267.3 rad/s, and its controller has gain **1297.91**, 2.3 times as much.

### acc90 — the ACC'90 spring-mass benchmark

**Source.** The ACC'90 benchmark problem; example 5 of the QFT Toolbox manual (Borghesani, Chait and Yaniv), where it is designed with a margin of 2.25; Nataraj and Kubal, IJRNC 17 (2007), ex. 4.1, p. 262-263, with margin specifications of their own.

**The problem.** `P(s) = e / (s² (s² + 0.02 s + 2e))`, `e ∈ [0.5, 2]`: two masses joined by an uncertain spring, a double integrator, a lightly damped pair at `√(2e)`. Stability margin **1.75** and nothing else, at ω = 0.1, 0.98, 0.99, 1, 2, 5, 7, 8.5, 10, 15, 20 and 100 rad/s. The three frequencies around 1 rad/s straddle the resonance, which sweeps from 1 to 2 rad/s across the family.

**Published controller.** Nataraj and Kubal, for their margins: `1.139·10⁷ (s+0.0751)(s+0.3488)(s+0.3868) / ((s+7.2019)(s+39.7899)(s+95.8659)(s+96.2539))`, three zeros and four poles.

**Why this structure.** Nataraj and Kubal's controller answers their own margin specifications, not the single stability margin posed here, so it is not the controller of this problem. With one zero and one pole no controller in the search box meets the margin and closes a stable loop with every plant: MC2 proves it. The file holds one zero and two poles, the smallest structure with a design, and a strictly proper one.

**The controller in the file.** 1 zero, 2 poles, gain **1000**, and every one of the 625 plants of the sweep closed-loop stable, the worst closed-loop pole at -0.001.

**How to read it.** 1000 is the floor of the search box, `[1000, 10⁸]`, Nataraj and Kubal's: with a stability margin as the only specification nothing holds the gain up, and at the design frequencies the margin is met with 22.7 dB to spare. The zero and the poles are the first feasible point the search reached, not an optimum: as posed, the problem has none inside the box.

**Below the margin's range.** The problem asks the margin from 0.1 to 100 rad/s, and at 0.0332 rad/s the design exceeds it by **19.7 dB**. Across the resonance band, from 0.9 to 2.1 rad/s, it is met on a grid of 20001 frequencies.

**The extended version.** `acc90-extended.qft` adds 0.03319 and 0.004266 rad/s, and its controller keeps the gain of **1000**, the floor of the box.

### dcm-k — the DC motor of Tharewal 3.1

**Source.** Tharewal 2005, example 3.1 (p. 37-38), the same as Nataraj and Tharewal, ASME JDSMC 2007, example 5.1; the design Chen, Ballance and Gawthrop 1998 made with a genetic algorithm is what it is compared against there.

**The problem.** `P(s) = k / (s (s + a))`, `k, a ∈ [1, 10]` - note **`k`, not `k·a`** in the numerator: this is Tharewal's plant, and its templates are not those of the QFT Toolbox's example 2 although the specifications are. Stability margin 1.2; tracking between `T_L = 120/(s³+17s²+82s+120)` and `T_U = 0.6584 (s+30)/(s²+4s+19.752)`, at ω = 0.1, 0.5, 1, 15 and 100 rad/s.

**Published controller.** `3462219 (s+3.85) / ((s+931.27)(s+946.83))`: one zero, two poles, a high-frequency gain of 3.46·10⁶, "a 48.73 % reduction" against Chen et al.

**The controller in the file.** 1 zero, 2 poles, gain **42527.4**.

**Between the design frequencies.** The margin is exceeded by **8.0 dB** at 54.2 rad/s, between 15 and 100.

**The extended version.** `dcm-k-extended.qft` adds 4.236, 4.325, 54.2, 62.66 and 63.97 rad/s, and its controller has gain **236142**, 5.6 times as much.

**How to read it.** Both controllers are zero-pole-gain forms with one more pole than zero, so the gain of each is its high-frequency gain and the numbers are comparable in kind: 4.3·10⁴ here, and 2.4·10⁵ with the margin at every frequency, against 3.5·10⁶ published, with the reservation that the paper's boundaries come from its own templates and tolerance. The tracking bound's coefficient is the correct **0.6584**; several papers carry 0.6854, a transposition that travels from one to the next.

### dcm-ka-w5 — the QFT Toolbox motor at Chait's frequencies

**Source.** Chait, Chen and Hollot, "Automatic loop-shaping of QFT controllers via linear programming", ASME JDSMC 121 (1999): the QFT Toolbox's example 2 at five design frequencies instead of six.

**The problem.** `P(s) = k a / (s (s + a))`, `k, a ∈ [1, 10]`; stability 1.2; tracking between `120/(s³+17s²+82s+120)` and `0.6584 (s+30)/(s²+4s+19.752)`; ω = 0.1, 0.5, 1, 15, 100 rad/s. Against `toolbox-2` the only change is the missing 2 rad/s.

**Published controller.** A controller of three degrees of freedom found by linear programming; the paper writes the tracking coefficient as 0.6854, a transposition of the Toolbox manual's 0.6584 - the one used here.

**The controller in the file.** 1 zero, 1 pole, gain **568.394**.

**How to read it.** The controller is `toolbox-2`'s to the last digit: dropping 2 rad/s changes nothing, the binding frequencies being elsewhere.

**Above the design frequencies.** As in `toolbox-2`, the margin is exceeded by **2.9 dB** at 217 rad/s.

**The extended version.** `dcm-ka-w5-extended.qft` adds 2.917, 3.516, 5.433, 217.3, 267.3 and 272.9 rad/s, and its controller has gain **1298.93**, 0.08 % from `toolbox-2-extended`'s.

### dcm-ka-w8 — the QFT Toolbox motor at Purohit's eight frequencies

**Source.** Purohit, Goldsztejn, Jermann, Granvilliers, Goualard and Nataraj, IJRNC 2016, experiment 4.1 (p. 10-12).

**The problem.** `P(s) = k a / (s (s + a))`, `k, a ∈ [1, 10]`, which the paper samples at 5 × 5 = 25 plants; stability 1.2; tracking between `120/(s³+17s²+82s+120)` and `0.6585 (s+30)/(s²+4s+19.752)`; ω = 0.5, 1, 2, 3, 5, 10, 30, 60 rad/s.

**Published controller.** A PID, `Kp = 9.22`, `Td = 0.41`, `Ti = 12.21`, with a prefilter. **The paper itself records that it violates the upper tracking bound over ω ∈ [11, 29] rad/s.**

**Why this structure.** The published controller is a PID, which has an integrator; QFTbx's controllers are real zeros, real poles and a gain, with no integrator, so the file holds the PID's two zeros over one real pole instead. Like the PID, that controller is improper: with two zeros over one pole its gain grows with frequency.

**The controller in the file.** 2 zeros, 1 pole, gain **0.326047**.

**Above the design frequencies.** The margin is exceeded by **2.7 dB** at 164 rad/s, above the last design frequency, 60.

**The extended version.** `dcm-ka-w8-extended.qft` adds 164 and 198.5 rad/s, and its controller has gain **0.680363**, 2.1 times as much.

**How to read it.** For a two-zero, one-pole controller the gain is the coefficient of `s` at high frequency, the counterpart of the PID's `Kd = Kp·Td = 3.78`.

### dcm-AC — the DC motor with the fourth-order lower bound

**Source.** IFAC DYCOPS 2013 (p. 431-432), the same problem as Tharewal 2005 example 3.2 (p. 38-39), against the optimal PID of Zolotas and Halikias.

**The problem.** `P(s) = k a / (s (s + a))`, `k, a ∈ [1, 10]`; stability 1.2; tracking between the fourth-order `8400/((s+3)(s+4)(s+10)(s+70))` and `0.6584 (s+30)/(s²+4s+19.752)` - the DYCOPS paper prints the two labels the other way round; ω = 0.5, 1, 2, 10, 30, 60 rad/s.

**Published controller.** The PID `7.03 + 3.89 s + 0.1/s`; Tharewal's is `12.1 + 3.53 s + 0.38/s`.

**Why this structure.** The published controllers are PIDs; QFTbx's controllers have no integrator, so the file holds two zeros over one real pole. Like the PIDs, that controller is improper: its gain grows with frequency.

**The controller in the file.** 2 zeros, 1 pole, gain **0.300228**.

**Above the design frequencies.** The margin is exceeded by **2.6 dB** at 155 rad/s, above the last design frequency, 60.

**The extended version.** `dcm-AC-extended.qft` adds 154.8, 183.9 and 187.4 rad/s, and its controller has gain **0.596137**, twice as much.

**How to read it.** The gain is the coefficient of `s` at high frequency, against the PID's `Kd = 3.89`.

### dcm-T33 — Tharewal 3.3, first-order bounds at very low frequencies

**Source.** Tharewal 2005, example 3.3 (p. 40-41) = ASME JDSMC 2007, example 5.3, against the linear-programming design of Bryant and Halikias.

**The problem.** `P(s) = k / (s (s + a))`, `k, a ∈ [1, 10]`; stability 1.2; tracking between `1/(s+1)²` and `1.5/(s+1.5)`; ω = 0.001, 0.0157, 0.2449, 3.8337 and 60 rad/s - three decades below 1 rad/s, where the bounds are nearly flat.

**Published controller.** `10455 (s+1.56)(s+1.29) / ((s+0.54)(s²+149.4s+17260))`, two zeros, one real pole and a complex pair, searched in a 30 % neighbourhood of Bryant and Halikias's parameters.

**Why this structure.** The published controller has a complex pole pair, which QFTbx's controllers - real zeros, real poles and a gain - cannot hold, so it cannot be posed as published; the file holds one zero and one pole.

**The controller in the file.** 1 zero, 1 pole, gain **41.1014**, and every one of the 625 plants of the sweep closed-loop stable. The zero sits at 1000, the top of the search box, over a pole at 468.7.

**Between the design frequencies.** The margin is exceeded by **43.2 dB** at 27.9 rad/s, between 3.8337 and 60, where a lightly damped closed-loop pair resonates.

**The extended version.** `dcm-T33-extended.qft` adds 2.4547, 2.455, 2.518, 21.68 and 27.93 rad/s, and its controller has gain **115.754**, 2.8 times as much. The first two are one peak: 2.455 is its frequency rounded, and 2.4547 the grid's own, added when the rounded one left less than 10⁻⁴ dB over the margin there.

**How to read it.** The gain is a high-frequency gain, against the paper's 10 455 - a different structure, so a comparison of kind and not of merit.

### dcm-hs72 — Horowitz and Sidi's corridor, by Bryant and Halikias

**Source.** Bryant and Halikias 1995, over the problem of Horowitz and Sidi 1972; Tharewal 2005 example 3.3 solves the same bounds on the plant `k/(s(s+a))` at five frequencies (`dcm-T33`).

**The problem.** `P(s) = k a / (s (s + a))`, `k, a ∈ [1, 10]`; the corridor `1/(s+1)² ≤ T ≤ 1.5/(s+1.5)`; 23 frequencies spaced logarithmically from 0.01 to 428.1 rad/s. The paper states no stability margin; **1.2 is added here**, as Tharewal does in 3.3.

**Published controller.** `13721 (s+1.21)(s+1) / ((s+0.599)(s²+150s+150²))`: two zeros, one real pole and a complex pair, found by linear programming.

**Why this structure.** The published controller has a complex pole pair, which QFTbx's controllers cannot hold; the file holds one zero and two poles.

**The controller in the file.** 1 zero, 2 poles, gain **26416.1**.

**At every frequency.** The design meets the stability margin at every frequency, not only at the 23 design ones, by 1.2 dB at the closest, so the problem has one file. Between the design frequencies the lower tracking bound is exceeded by up to 0.49 dB, at 81 rad/s.

**How to read it.** The gain, one more pole than zero, is a high-frequency gain, against the paper's 13 721 - a different structure, so a comparison of kind and not of merit.

### msf — the MSF desalination plant of Tharewal 4.4

**Source.** Tharewal 2005, example 4.4 (p. 68-72): the top-brine-temperature loop of a multi-stage flash desalination plant, over the model of Ismail. Tharewal uses it for an integer-order against a fractional-order controller.

**The problem.** `P(s) = K (1 + T1 s) / ((1 + T2 s)(1 + T3 s))`, four uncertain parameters: `K ∈ [32, 76]`, `T1 ∈ [12, 28]`, `T2 ∈ [11, 26]`, `T3 ∈ [4, 10]`; nominal `54 (1 + 20.32 s)/((1 + 18.3 s)(1 + 7.2 s))`. Stability margin 1.2; tracking between `60.44/(s³ + 13.5 s² + 50.94 s + 60.44)` and `(16.32 s + 197.9)/(12.12 s² + 64.65 s + 197.9)`, which the authors add themselves ("Ismail does not use any tracking specifications"); ω = 0.01, 0.098, 0.309, 0.97, 9.558, 30 rad/s. A stable plant with no integrator and slow time constants.

**Published controller.** `655.65 (s + 2.022) / (s (s + 169.9))`: one zero, an integrator and one pole, searched in `{[1, 10⁸], [0.001, 50], [0.001, 1500], [0.001, 1500]}`.

**Why this structure.** The published controller has an integrator, which QFTbx's controllers do not; the file holds the same zero over two real poles.

**The controller in the file.** 1 zero, 2 poles, gain **233.738**.

**Above the design frequencies.** The margin is exceeded by **3.8 dB** at 95.5 rad/s, above the last design frequency, 30.

**The extended version.** `msf-extended.qft` adds 3.802, 4.467, 95.5, 173.8 and 195 rad/s, and its controller has gain **1484.59**, 6.4 times as much.

**How to read it.** With one more pole than zero the gain is a high-frequency gain, against the paper's 655.65 with its integrator. The integrator is what gives the published design zero steady-state error; the design here does not have it, and reads as a frequency-domain comparison only.

### maglev-lower — Purohit's maglev on the stable side of its equilibrium

**Source.** Purohit, Goldsztejn, Jermann, Granvilliers, Goualard and Nataraj, IJRNC 2016, experiment 4.2 (p. 12-13): a magnetic levitation system linearised on either side of its equilibrium. This is the half whose poles are on the imaginary axis; `maglev-upper` is the unstable one.

**The problem.** `P(s) = k / (s² + a)`, `k ∈ [811, 944]`, `a ∈ [382, 478.5]`: a pair of poles on the imaginary axis at `±j√a ≈ ±20 rad/s` at every member, so no member is asymptotically stable on its own. Stability margin 1.2; tracking between `916.3/(s³ + 39.76 s² + 354.9 s + 916.3)` and `(1.722 s + 68.89)/(s² + 16.6 s + 68.89)`; eleven frequencies from 0.1 to 30 rad/s.

**Published controller.** A PID with a second-order filter, the same family as for the unstable half.

**Why this structure.** A PID with a filter has an integrator and a complex pole pair, and QFTbx's controllers hold neither; the file holds two zeros and one pole. Unlike the filtered PID, that controller is improper: its gain grows with frequency.

**The controller in the file.** 2 zeros, 1 pole, gain **0.01**, and every one of the 625 plants of the sweep closed-loop stable, the worst closed-loop pole at -0.35.

**How to read it.** The gain is the floor of the search box, `[0.01, 10⁸]`: the specifications are met by any gain that small, so the number is the smallest the box allows and not the smallest the problem allows. The plant already carries a gain above 800, which is where the loop's magnitude comes from.

**Above the margin's range.** The problem asks the margin from 0.1 to 30 rad/s, and at 92.9 rad/s the design exceeds it by **39.6 dB**.

**The extended version.** `maglev-lower-extended.qft` asks the margin from 0.001 to 10⁶ rad/s, adds 36.56, 43.15, 58.88, 92.9 and 208.4 rad/s, and its controller has gain **0.0821922**, off the floor of the box.

### maglev-upper — the unstable half of Purohit's maglev

**Source.** Purohit, Goldsztejn, Jermann, Granvilliers, Goualard and Nataraj, IJRNC 2016, experiment 4.2 (p. 12-13): a magnetic levitation system linearised on either side of its equilibrium.

**The problem.** `P(s) = k / (s² − a)`, `k ∈ [1021, 1106]`, `a ∈ [382, 478.5]`: one real pole in the right half-plane at every member, at `√a ≈ 20 rad/s`. Stability margin 1.2; tracking between `916.3/(s³ + 39.76 s² + 354.9 s + 916.3)` and `(1.722 s + 68.89)/(s² + 16.6 s + 68.89)`; eleven frequencies from 0.1 to 30 rad/s.

**Published controller.** A PID with a second-order filter, `Kp = 2.956`, `Td = 0.0604`, `Ti = 0.4195`, `ωn = 5.0075`, `ζ = 0.995`.

**Why this structure.** A PID with a second-order filter has an integrator and a complex pole pair, neither of which QFTbx's controllers hold; the file holds one zero and one pole.

**The controller in the file.** 1 zero, 1 pole, gain **708.503**.

**How to read it.** The zero and the pole almost cancel, 726.5651 against 726.5654, so the controller is in effect a static gain. With `k/(s² − a)` a static gain `C` closes the loop as `s² + (kC − a)`: two poles on the imaginary axis at `±j√(kC − a)`, 850 to 885 rad/s across the family, which the zero and the pole are left to damp. The problem asks the margin up to 30 rad/s, and at 853 rad/s the design exceeds it by **72.7 dB**.

**The extended version.** With the margin asked at every frequency from 0.001 to 10⁶ rad/s, no controller with one zero and one pole exists in the search box: as frequencies are added the resonance moves up, from 853 to 2.2·10⁵ rad/s with the gain rising from 708.5 to 4.6·10⁷, until MC2 proves that none is left. `maglev-upper-extended.qft` holds two zeros and two poles instead. It adds 13.24, 15, 15.63, 89.13, 100.9 and 107.4 rad/s, and its controller has gain **38.4827**, zeros at 3.18 and 19.66 and poles at 0.01 and 257.8, the one at the floor of the box standing in for the PID's integrator; the worst closed-loop pole of the family is at -3.69. With two zeros and two poles at the paper's frequencies alone the gain would be 8.16, with the margin exceeded by 83 dB at 89 rad/s.

### fopdt — first order plus delay, Purohit 4.3

**Source.** Purohit et al., IJRNC 2016, experiment 4.3 (p. 15-16).

**The problem.** A first-order plant with a delay, the delay written as a first-order Padé: `P(s) = k (1 − td s/2) / ((s + a)(1 + td s/2))`, `k ∈ [1, 3]`, `a ∈ [1, 2]`, `td ∈ [0.08, 0.12]`. The Padé puts a zero in the right half-plane at `2/td`, between 16.7 and 25 rad/s, which is what limits the bandwidth. Stability margin 1.2; tracking between `9/(s³ + 7 s² + 15 s + 9)` and `4/(s² + 3.3 s + 4)`; ω = 0.1, 0.2, 0.5, 1, 2, 5, 8, 10 and 50 rad/s.

**Published controller.** A PID, `Kp = 1.88`, `Td = 0.05`, `Ti = 0.72`.

**Why this structure.** A PID has an integrator, which QFTbx's controllers do not hold; the file holds two zeros and one pole. Like the PID, that controller is improper: its gain grows with frequency.

**The controller in the file.** 2 zeros, 1 pole, gain **0.0639392**.

**How to read it.** Three uncertain parameters, so the template is a volume sampled at nine points on each: the problem here that costs the most template. The zero in the right half-plane cannot be cancelled, so the loop has to be shaped below it; the two zeros of the controller, at 2.09 and 31.62, sit on either side of it.

**Between the design frequencies.** The margin is exceeded by **0.0008 dB** at 4.90 rad/s, between 2 and 5.

**The extended version.** `fopdt-extended.qft` adds 4.898 rad/s, and its controller has gain **0.0647**, 1.2 % more.

### unstable — Tharewal's unstable plant, stability alone

**Source.** Tharewal 2005, example 3.6 (p. 47-48), designed directly on the unstable nominal plant rather than on a stabilised one, against Chen and Ballance's hand design.

**The problem.** `P(s) = k (s + a) / (s² − 2.5)`, `k ∈ [1, 10]`, `a ∈ [0.1, 1]`, nominal `(s+1)/(s²−2.5)`: one pole in the right half-plane at every member. Stability margin **2.1** and nothing else, at ω = 0.1, 1, 2, 6 and 50 rad/s.

**Published controller.** A static gain, **5.18**, searched in `(0, 10⁸]`; Chen and Ballance's is 6.582.

**Why this structure.** The published controller is a static gain, and no static gain stabilises the whole family (below); the file holds one zero and one pole, which does.

**What the published gain does.** With `C` a constant the closed loop's characteristic polynomial is `s² + kC s + (kC a − 2.5)`, stable if and only if `kC a > 2.5`. At `k = 1` that needs `C > 2.5/a`: **5.18 leaves every member with `a < 0.48` unstable**, and 6.582 every one with `a < 0.38`. No static gain below 25 stabilises the worst member, `k = 1, a = 0.1`. The margin at the five design frequencies does not see this - it is met by gains from about 20 - and that gap, between a margin sampled at the design frequencies and the stability of every member, is why every controller here is also checked against the closed-loop poles of every plant of the sweep.

**The controller in the file.** 1 zero, 1 pole, gain **0.0729821**, and every one of the 625 plants of the sweep closed-loop stable. The zero sits at 1000, the top of the search box, over a pole at 2.90.

**Below the design frequencies.** Near zero frequency the margin is exceeded by **32.1 dB**: the loop gain of the member `k = 1, a = 0.1` is -1.007 at zero frequency, and the worst closed-loop pole of the family is at -0.0007.

**The extended version.** `unstable-extended.qft` adds 0.001, 3.17, 4.236, 11.22, 20.89, 90.99, 107.4, 140.6 and 149.6 rad/s, and its controller has gain **2.65673**, 36 times as much.
