# Design problems from the literature, solved with QFTbx

Each file here is a published QFT design problem: the plant and its
uncertainty, the specifications, the design frequencies and the structure of
the controller are the paper's, except where a page below says otherwise, and
the file carries everything QFTbx computed from them - the templates and their
contours, the boundaries, the controller found and the verifier's verdict on
it, the closed-loop stability of every plant of the sweep included. Each file records the sweep its templates came from, so
the verdict can be recomputed from the file alone.

Open any of them with QFTbx to see every phase.

The plants are grouped by the family they come from. Seven are the DC motor
`k a/(s(s+a))` of the QFT Toolbox's example 2, with the specifications and the
design frequencies of each paper.

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
`toolbox-1`, as the manual poses it, meets its stability margin at its four
frequencies and exceeds it by 35.7 dB at 57.7 rad/s, where a lightly damped
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
says which of the two it is. `dcm-T33` and `dcm-hs72` meet their margin at every
frequency as posed, and have one file each; `toolbox-1` has one file too, for
the reason its page gives. `acc90` meets it as posed as well, and its extended
version poses the benchmark with its resonance undamped.

## The problems

| Problem | Source | Plant and uncertainty | Specifications | Design frequencies | Published controller | Controller in the file |
|---|---|---|---|---|---|---|
| `toolbox-1` | QFT Toolbox manual (Borghesani, Chait and Yaniv), example 1 | `k/((s+a)(s+b))`; `k ∈ [1,10]`, `a ∈ [1,5]`, `b ∈ [20,30]` | stability `1.2`; output disturbance `0.02(s³+64s²+748s+2400)/(s²+14.4s+169)` over [0,10]; input disturbance `0.01` over [0,50] | 0.1 5 10 100 | designed by hand in the manual | 1 zero, 1 pole, gain 50.2204 |
| `toolbox-2` | QFT Toolbox manual, example 2 - the problem most of the rest descend from | `k a/(s(s+a))`; `k,a ∈ [1,10]` | stability `1.2`; corridor `120/(s³+17s²+82s+120)` to `0.6584(s+30)/(s²+4s+19.752)` | 0.1 0.5 1 2 15 100 | designed by hand in the manual; second order by genetic algorithm in Chen, Ballance and Gawthrop 1998 (`k_hf` 136.76 dB); CRONE-2 in Cervera and Baños 2008 (`K_hf` 129.75 dB) | 1 zero, 1 pole, gain 568.394 |
| `acc90` | ACC'90 benchmark, spring-mass, as Martínez-Forte and Cervera pose it, IJRNC 31 (2021), sec. 4.2 (p. 16); the controller's structure and box of Nataraj and Kubal, IJRNC 17 (2007), ex. 4.1 (p. 262-263) | `e/(s²(s²+0.01s+2e))`; `e ∈ [0.5,2]` — a double integrator and a lightly damped resonance | stability `1.75` and nothing else | 0.1 0.98 0.99 1 2 5 7 8.5 10 15 20 100 | Nataraj and Kubal: `1.139e7(s+0.0751)(s+0.3488)(s+0.3868)/((s+7.2019)(s+39.7899)(s+95.8659)(s+96.2539))`; the paper's, the same with two more zeros and poles | 3 zeros, 4 poles, gain 10⁷ |
| `acc90-toolbox` | the same benchmark as the QFT Toolbox manual poses it, example 5 (p. 5-7) | `e/(s²(s²+2e))`; `e ∈ [0.5,2]` — the resonance undamped | stability `2.25`; sensitivity `0.2` from 0.01 to 0.05 rad/s, added here | 0.01 0.05 0.1 0.9 0.99 1.01 1.5 2.01 20 | `0.032(10s+1)/(4s²+2s+1)` — **a complex pole pair** | 1 zero, 2 poles, gain 0.017526 |
| `dcm-k` | Tharewal 2005, ex. 3.1 (p. 37-38) = Nataraj and Tharewal, ASME 2007, ex. 5.1 | `k a/(s(s+a))`, `k,a ∈ [1,10]` | stability `1.2`; tracking `T_U = 0.6584(s+30)/(s²+4s+19.752)`, `T_L = 120/(s³+17s²+82s+120)` | 0.1 0.5 1 15 100 | `3462219(s+3.85)/((s+931.27)(s+946.83))` — 1 zero, 2 poles | 1 zero, 2 poles, gain 399855 |
| `dcm-ka-w5` | Chait, Chen and Hollot, ASME JDSMC 121 (1999) | `k a/(s(s+a))`, `k,a ∈ [1,10]` | the same as `dcm-k` | 0.1 0.5 1 15 100 | three degrees of freedom by linear programming | 1 zero, 1 pole, gain 568.394 |
| `dcm-ka-w8` | Purohit, Goldsztejn, Jermann, Granvilliers, Goualard and Nataraj, IJRNC 2016, exp. 4.1 (p. 10-12) | `k a/(s(s+a))`, `k,a ∈ [1,10]` | stability `1.2`; the same corridor | 0.5 1 2 3 5 10 30 60 | PID `Kp 9.22, Td 0.41, Ti 12.21` and a prefilter; **the paper admits it violates the upper tracking bound over ω ∈ [11,29]** | 2 zeros, 1 pole, gain 0.326047 |
| `dcm-AC` | IFAC DYCOPS 2013 (p. 431-432); Tharewal 2005, ex. 3.2 | `k a/(s(s+a))`, `k,a ∈ [1,10]` | stability `1.2`; tracking with the fourth-order lower bound `8400/((s+3)(s+4)(s+10)(s+70))` | 0.5 1 2 10 30 60 | PID `7.03 + 3.89s + 0.1/s` | 2 zeros, 1 pole, gain 0.300228 |
| `dcm-T33` | Tharewal 2005, ex. 3.3 (p. 40-41) = ASME 2007, ex. 5.3 | `k a/(s(s+a))`, `k,a ∈ [1,10]` | stability `1.2`; tracking `T_U = 1.5/(s+1.5)`, `T_L = 1/(s+1)²` | 0.001 0.0157 0.2449 3.8337 60 | `10455(s+1.56)(s+1.29)/((s+0.54)(s²+149.4s+17260))` — **a complex pole pair** | 1 zero, 1 pole, gain 68.524 |
| `dcm-hs72` | Bryant and Halikias 1995, over Horowitz and Sidi 1972 | `k a/(s(s+a))`, `k,a ∈ [1,10]` | the corridor `1/(s+1)² ≤ T ≤ 1.5/(s+1.5)`; the paper states no margin, `1.2` added here as Tharewal 3.3 does | 23 logarithmic, 0.01 to 428.1 | by linear programming | 1 zero, 2 poles, gain 26416.1 |
| `msf` | Tharewal 2005, ex. 4.4 (p. 68-72) | MSF desalination `K(1+T1 s)/((1+T2 s)(1+T3 s))`; `K ∈ [32,76]`, `T1 ∈ [12,28]`, `T2 ∈ [11,26]`, `T3 ∈ [4,10]` | stability `1.2`; tracking added by the authors ("Ismail does not use any tracking specifications") | 0.01 0.098 0.309 0.97 9.558 30 | `655.65(s+2.022)/(s(s+169.9))` — **an integrator** | 1 zero, 2 poles, gain 233.738 |
| `maglev-lower` | Purohit, Goldsztejn, Jermann, Granvilliers, Goualard and Nataraj, IJRNC 2016, exp. 4.2 (p. 12-13) | `k/(s²+a)`; `k ∈ [811,944]`, `a ∈ [382,478.5]` — poles on the imaginary axis | stability `1.2`; corridor `916.3/(s³+39.76s²+354.9s+916.3)` to `(1.722s+68.89)/(s²+16.6s+68.89)` | 11 from 0.1 to 30 | PID with `ωn`, `ζ` | 2 zeros, 1 pole, gain 0.01 |
| `maglev-upper` | the same paper, the unstable half | `k/(s²−a)`; `k ∈ [1021,1106]`, `a ∈ [382,478.5]` | the same | the same | PID with `ωn`, `ζ` | 1 zero, 1 pole, gain 708.503 |
| `fopdt` | the same paper, exp. 4.3 (p. 15-16) | first order plus delay, first-order Padé: `k(1−td s/2)/((s+a)(1+td s/2))`; `k ∈ [1,3]`, `a ∈ [1,2]`, `td ∈ [0.08,0.12]` | stability `1.2`; corridor `9/(s³+7s²+15s+9)` to `4/(s²+3.3s+4)` | 0.1 0.2 0.5 1 2 5 8 10 50 | PID `Kp 1.88, Td 0.05, Ti 0.72` | 2 zeros, 1 pole, gain 0.0639392 |
| `unstable` | Tharewal 2005, ex. 3.6 (p. 47-48) | unstable `k(s+a)/(s(s−2.5))`; `k ∈ [1,10]`, `a ∈ [0.1,1]` | stability `2.1` and nothing else ("the only design spec considered") | 0.1 1 2 6 50 | `5.18` — a static gain, searched in `(0,10⁸]`; Chen and Ballance's hand design, `6.582` | a static gain, 5.20471 |

## The extended versions

The margin as posed is the worst excess over the stability margin on the grid of
1001 frequencies, with the frequency it happens at.

| Problem | Margin as posed | Frequencies added (rad/s) | Gain as posed | Gain extended |
|---|---|---|---|---|
| `toolbox-1` | +35.7 dB at 57.7 | none found yet: see its page | 50.2204 | - |
| `toolbox-2` | +2.9 dB at 217 | 2.917 3.516 5.433 217.3 267.3 | 568.394 | 1297.91 |
| `acc90` | met, by 0.30 dB | the plant undamped, with 1 and 2 moved to 1.01 and 2.01; then 1.197 1.633 | 10⁷ | 10⁷ |
| `acc90-toolbox` | +32.7 dB at 0.151 | 0.1514 0.166 0.1738 0.1778 0.1862 0.1906 0.2042 0.2291 | 0.017526 | 0.0675908 |
| `dcm-k` | +10.7 dB at 262 | 2.917 3.041 3.589 5.433 261.8 357.3 364.8 | 399855 | 3.21804·10⁶ |
| `dcm-ka-w5` | +2.9 dB at 217 | 2.917 3.516 5.433 217.3 267.3 272.9 | 568.394 | 1298.93 |
| `dcm-ka-w8` | +2.7 dB at 164 | 164 198.5 | 0.326047 | 0.680363 |
| `dcm-AC` | +2.6 dB at 155 | 154.8 183.9 187.4 | 0.300228 | 0.596137 |
| `dcm-T33` | met, by 0.34 dB | none: one file | 68.524 | - |
| `dcm-hs72` | met, by 1.2 dB | none: one file | 26416.1 | - |
| `msf` | +3.8 dB at 95.5 | 3.802 4.467 95.5 173.8 195 | 233.738 | 1484.59 |
| `maglev-lower` | +39.6 dB at 92.9 | 36.56 43.15 58.88 92.9 208.4 | 0.01 | 0.0821922 |
| `maglev-upper` | +72.7 dB at 853 | 13.24 15 15.63 89.13 100.9 107.4, with two zeros and two poles | 708.503 | 38.4827 |
| `fopdt` | +0.0008 dB at 4.90 | 4.898 | 0.0639392 | 0.0647 |
| `unstable` | +0.05 dB at 2.14 | 2.138 | 5.20471 | 5.23401 |

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

**Published controllers.** The manual's, by hand. Chen, Ballance and Li 1998, second order by genetic algorithm, `k_hf` 6.753·10⁶ (136.59 dB). Cervera and Baños 2008, CRONE-2 with `n_pe = 3`, `K_hf` 129.75 dB. The text of the manual's example 2 prints the tracking coefficient as 0.6854 and the denominator coefficient 82 as "828", while its code and its reference pages carry 0.6584 and 82; Chen, Ballance and Li carry the two misprints over. The corridor here is the code's.

**The controller in the file.** 1 zero, 1 pole, gain **568.394**.

**Above the design frequencies.** The margin holds up to the last design frequency, 100 rad/s, and is exceeded by **2.9 dB** at 217 rad/s.

**The extended version.** `toolbox-2-extended.qft` adds 2.917, 3.516, 5.433, 217.3 and 267.3 rad/s, and its controller has gain **1297.91**, 2.3 times as much.

### acc90 — the ACC'90 spring-mass benchmark, as Martínez-Forte and Cervera pose it

**Source.** Martínez-Forte and Cervera, IJRNC 31 (2021), section 4.2 (p. 16), and chapter 6 of the thesis behind it; the benchmark is Wie and Bernstein's (ACC 1990). The controller's structure and search box are those of Nataraj and Kubal, IJRNC 17 (2007), example 4.1 (p. 262-263).

**The problem.** `P(s) = e / (s² (s² + 0.01 s + 2e))`, `e ∈ [0.5, 2]`, nominal 0.5: two masses joined by an uncertain spring, a double integrator and a resonance at `√(2e)`, which sweeps from 1 to 2 rad/s across the family. The benchmark's resonance is undamped; the paper damps it lightly to plot the loop in a finite area of the Nichols chart, without giving the damping, and `0.01 s` is the damping the heights of the thesis's templates match. Stability margin **1.75** and nothing else, at 0.1, 0.98, 0.99, 1, 2, 5, 7, 8.5, 10, 15, 20 and 100 rad/s.

**Published controller.** Nataraj and Kubal's, `1.139·10⁷ (s+0.0751)(s+0.3488)(s+0.3868) / ((s+7.2019)(s+39.7899)(s+95.8659)(s+96.2539))`; the paper's is the same with two more zeros (25, 35) and poles (30, 40).

**Why this structure.** The loop of this paper, and of Nataraj and Kubal, passes the band from 1 to 2 rad/s above 0 dB, and with the resonance undamped it must hold there a phase lead of more than 180°, or the resonance crosses the −180° ray at infinite magnitude (Cervera and Baños, JVC 2013). Real zeros lead by less than 90° each, so the controller needs three: the file holds Nataraj and Kubal's three zeros and four poles, each searched within 90% of the seed they started from, with the gain in their `[10⁷, 10¹⁰]`. With one zero and two poles and a gain from 1000, the design the search returns keeps the resonance under 0 dB, which the damping allows and the undamped benchmark does not: it leaves every undamped plant unstable.

**The controller in the file.** 3 zeros (0.1323, 0.1428, 0.6045), 4 poles (7.202, 39.79, 50.66, 318.8), gain **10⁷**, and every one of the 625 plants of the sweep closed-loop stable, the worst closed-loop pole at −0.053. With the plant undamped the loop is stable too, over 3001 values of `e`, the worst pole again at −0.053.

**How to read it.** 10⁷ is the floor of the gain box, as Nataraj and Kubal's own gain is near it: with a stability margin as the only specification nothing holds the gain up.

**At every frequency.** The design meets the margin over the grid of 1001 frequencies as posed, by 0.30 dB.

**The extended version.** `acc90-extended.qft` poses the benchmark as it is, undamped, with the margin from 0.001 to 10⁶ rad/s. Then 1 rad/s is a pole of the nominal plant and 2 rad/s a pole of the stiffest plant, so they become 1.01 and 2.01, as the QFT Toolbox does; at 1.01 rad/s the template is unbounded, since a plant between the sampled ones resonates there, and the templates window says so; and the parameter is swept at 602 points, so that no sampled plant falls exactly on a design frequency. It adds 1.197 and 1.633 rad/s, and its controller has the zeros 0.1428, 0.2406 and 0.8222, the poles 31.88, 39.79, 50.66 and 72.02 and gain **10⁷**; the worst closed-loop pole is at −0.086.

### acc90-toolbox — the same benchmark as the QFT Toolbox poses it

**Source.** Example 5 of the QFT Toolbox manual (Borghesani, Chait and Yaniv), p. 5-7, and its demo `qftex5.m`.

**The problem.** The plant of `acc90`, undamped, nominal 0.5. Stability margin **2.25** at every frequency, which the manual found appropriate in place of the benchmark's time-domain specifications; design frequencies 0.01, 0.1, 0.9, 0.99, 1.01, 1.5, 2.01 and 20 rad/s, three of them inside the resonance band and none on a resonance of the sweep.

**What differs from the manual.** A stability margin alone admits ever smaller gains: the same loop slid to lower frequencies keeps the margin while its gain goes to zero. So a bound on the sensitivity, `|1/(1+L)| ≤ 0.2` from 0.01 to 0.05 rad/s, is added here, with 0.05 rad/s among the design frequencies; the designs of the manual, of Horowitz, of Nataraj and Kubal and of Martínez-Forte and Cervera all meet it. The controller is searched with the gain in `[10⁻⁴, 10]` and the zero and the poles in `[0.01, 10]`, and the parameter is swept at 602 points.

**Published controller.** `0.032 (10s+1)/(4s²+2s+1)`: one zero and a complex pole pair.

**Why this structure.** This is the other branch: the loop crosses 0 dB below the resonance band and passes it at low gain, with a phase lag, so that the resonance crosses the positive real axis and not the −180° ray. One zero and two poles hold it; QFTbx's poles are real, so the manual's complex pair becomes two real poles.

**The controller in the file.** 1 zero (0.01), 2 poles (0.0862, 0.309), gain **0.017526**, and every one of the 602 plants closed-loop stable, the worst closed-loop pole at −3.3·10⁻⁶.

**How to read it.** The least gain leaves a closed-loop pair of the rigid mode on the imaginary axis, at 0.15 rad/s, between the design frequencies 0.1 and 0.9: there the design exceeds the margin by **32.7 dB**.

**The extended version.** `acc90-toolbox-extended.qft` asks the margin from 0.0001 to 10⁶ rad/s and adds eight frequencies between 0.15 and 0.23 rad/s. Its controller has the zero at 0.0649, the poles at 0.425 and 0.437 and gain **0.0675908**, crosses 0 dB at about 0.17 rad/s against the manual's 0.20, and its worst closed-loop pole is at −0.0038.

### dcm-k — the DC motor of Tharewal 3.1

**Source.** Tharewal 2005, example 3.1 (p. 37-38), the same as Nataraj and Tharewal, ASME JDSMC 2007, example 5.1; the design Chen, Ballance and Li 1998 made with a genetic algorithm is what it is compared against there.

**The problem.** `P(s) = k a / (s (s + a))`, `k, a ∈ [1, 10]`; stability margin 1.2; tracking between `T_L = 120/(s³+17s²+82s+120)` and `T_U = 0.6584 (s+30)/(s²+4s+19.752)`, at ω = 0.1, 0.5, 1, 15 and 100 rad/s. It is the problem of `dcm-ka-w5`, posed here with the paper's one zero and two poles.

**Published controller.** `3462219 (s+3.85) / ((s+931.27)(s+946.83))`: one zero, two poles, a high-frequency gain of 3.46·10⁶, "a 48.73 % reduction" against Chen et al.

**The controller in the file.** 1 zero, 2 poles, gain **399855**, the zero at 1.81 and the poles at 313.4 and 315.9.

**Above the design frequencies.** The margin is exceeded by **10.7 dB** at 262 rad/s, above the last design frequency, 100.

**The extended version.** `dcm-k-extended.qft` adds 2.917, 3.041, 3.589, 5.433, 261.8, 357.3 and 364.8 rad/s, and its controller has gain **3.21804·10⁶**, 8 times as much, with the zero at 3.79 and the poles at 904.3 and 905.5.

**How to read it.** Both controllers are zero-pole-gain forms with one more pole than zero, so the gain of each is its high-frequency gain and the numbers are comparable: 4.0·10⁵ as posed, and 3.2·10⁶ with the margin at every frequency, against the published 3.46·10⁶, whose zero at 3.85 and poles at 931 and 947 are close to those of the extended design. The paper's boundaries come from its own templates and tolerance. The tracking bound's coefficient is the correct **0.6584**; the 0.6854 several papers carry comes from the text of the Toolbox manual (see `toolbox-2`).

### dcm-ka-w5 — the QFT Toolbox motor at Chait's frequencies

**Source.** Chait, Chen and Hollot, "Automatic loop-shaping of QFT controllers via linear programming", ASME JDSMC 121 (1999): the QFT Toolbox's example 2 at five design frequencies instead of six.

**The problem.** `P(s) = k a / (s (s + a))`, `k, a ∈ [1, 10]`; stability 1.2; tracking between `120/(s³+17s²+82s+120)` and `0.6584 (s+30)/(s²+4s+19.752)`; ω = 0.1, 0.5, 1, 15, 100 rad/s. Against `toolbox-2` the only change is the missing 2 rad/s.

**Published controller.** A controller of three degrees of freedom found by linear programming. The paper prints the corridor with the 0.6854 and the "828" of the manual's text (see `toolbox-2`) and swaps the labels of its upper and lower bounds; the corridor here is the manual's code's.

**The controller in the file.** 1 zero, 1 pole, gain **568.394**.

**How to read it.** The controller is `toolbox-2`'s to the last digit: dropping 2 rad/s changes nothing, the binding frequencies being elsewhere. `dcm-k` is the same problem with one more pole.

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

**The problem.** `P(s) = k a / (s (s + a))`, `k, a ∈ [1, 10]`; stability 1.2; tracking between `1/(s+1)²` and `1.5/(s+1.5)`; ω = 0.001, 0.0157, 0.2449, 3.8337 and 60 rad/s - three decades below 1 rad/s, where the bounds are nearly flat.

**Published controller.** `10455 (s+1.56)(s+1.29) / ((s+0.54)(s²+149.4s+17260))`, two zeros, one real pole and a complex pair, searched in a 30 % neighbourhood of Bryant and Halikias's parameters.

**Why this structure.** The published controller has a complex pole pair, which QFTbx's controllers - real zeros, real poles and a gain - cannot hold, so it cannot be posed as published; the file holds one zero and one pole.

**The controller in the file.** 1 zero, 1 pole, gain **68.524**, the zero at 1.34 and the pole at 68.1, and every one of the 625 plants of the sweep closed-loop stable.

**At every frequency.** The design meets the stability margin at every frequency, not only at the five design ones, by 0.34 dB at the closest, at 67.8 rad/s, so the problem has one file.

**How to read it.** The gain is a high-frequency gain, against the paper's 10 455 - a different structure, so a comparison of kind and not of merit.

### dcm-hs72 — Horowitz and Sidi's corridor, by Bryant and Halikias

**Source.** Bryant and Halikias 1995, over the problem of Horowitz and Sidi 1972; Tharewal 2005 example 3.3 solves the same bounds on the same plant at five frequencies (`dcm-T33`).

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

**The problem.** `P(s) = k (s + a) / (s (s − 2.5))`, `k ∈ [1, 10]`, `a ∈ [0.1, 1]`, nominal `(s+1)/(s(s−2.5))`: an integrator and a pole in the right half-plane at every member. Stability margin **2.1** and nothing else, at ω = 0.1, 1, 2, 6 and 50 rad/s.

**Published controller.** A static gain, **5.18**, searched in `(0, 10⁸]`; Chen and Ballance's is 6.582.

**What the published gains do.** With `C` a constant the closed loop's characteristic polynomial is `s² + (kC − 2.5) s + kC a`, stable if and only if `kC > 2.5`: with `k ≥ 1`, every gain above 2.5 stabilises the whole family, 5.18 and 6.582 included. Over the 625 plants of the sweep and at the five design frequencies, 5.18 exceeds the margin by 0.043 dB at 2 rad/s, on the member `k = 1, a = 1`, and 6.582 meets it by 1.73 dB.

**The controller in the file.** A static gain, as the paper poses it: **5.20471**, the least that meets the margin at the design frequencies, with every one of the 625 plants of the sweep closed-loop stable.

**Between the design frequencies.** The margin is exceeded by **0.05 dB** at 2.14 rad/s, between 2 and 6.

**The extended version.** `unstable-extended.qft` adds 2.138 rad/s, and its gain is **5.23401**, 0.6 % more.
