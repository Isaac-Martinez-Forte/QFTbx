# The specifications

A QFT design is a loop that meets a set of requirements over every plant of
the family, at every design frequency. QFTbx supports the six restrictions
the literature states, in seven fixed slots — tracking takes two, one per
side of its band. The interface fills the slots, the persistence writes them
in that order, and the boundary engine and the checker index them by type. A
slot nobody filled is simply not used.

## What each one requires

With \( P \) any plant of the family, \( C \) the controller, \( L = PC \)
the loop of that plant, \( F \) the prefilter, and each bound a constant or
a transfer function evaluated at each frequency:

| Slot | Requirement | |
|---|---|---|
| `TrackingLower` / `TrackingUpper` | \( \alpha(\omega) \leq \left\| F \dfrac{L}{1+L} \right\| \leq \beta(\omega) \) | (1.6) |
| `Stability` | \( \left\| \dfrac{L}{1+L} \right\| \leq \lambda \) | (1.7) |
| `SensorNoise` | \( \left\| \dfrac{L}{1+L} \right\| \leq \delta_n(\omega) \) | (1.8) |
| `OutputDisturbance` | \( \left\| \dfrac{1}{1+L} \right\| \leq \delta_{po}(\omega) \) | (1.9) |
| `InputDisturbance` | \( \left\| \dfrac{P}{1+L} \right\| \leq \delta_{pi}(\omega) \) | (1.10) |
| `ControlEffort` | \( \left\| \dfrac{C}{1+L} \right\| \leq \delta_{ce}(\omega) \) | (1.11) |

The numbers are the equations of I. Martínez Forte, *Diseño automático de
controladores en QFT*, PhD thesis, 2022, section 1.1.4, which states the
list as the QFT literature does (Horowitz; Houpis, Rasmussen and
García-Sanz; the survey in `documentos/Tesis/3-estado-del-arte/`). Every one
of them is a bound on the magnitude of a closed-loop transfer function, and
every one has to hold for the whole plant family.

Robust stability and sensor noise share a magnitude and differ only in their
bound, and that is not an accident of the implementation: noise at the
sensor reaches the output through the closed loop, which is the same
transfer the M circle bounds.

## What the program computes

The same six, in one place
(`src/core/boundaries/closed_loop_worst_case.h`): the boundary sweep
evaluates them at every point of the Nichols grid to build its sheets, and
the checker evaluates them once at the loop of a returned controller to
verify it. The interface draws them from the same definition, which is why
the table of the specifications form reads the way it does.

Two differences with the list above, both deliberate:

- **There is no prefilter.** \( F \) multiplies the whole closed loop at
  each frequency, so it shifts the band of \( |FL/(1+L)| \) up or down but
  cannot narrow it. What the loop shaping has to achieve is therefore that
  the SPREAD of \( |L/(1+L)| \) over the plant family fit inside
  \( \beta - \alpha \); \( F \) then slides that spread into place. QFTbx
  computes the tracking boundary against that difference
  (`SpecificationSet::trackingSpreadDb`) and the checker verifies the same
  thing. Designing \( F \) is step 6 of the QFT flow and the toolbox does
  not do it yet.
- **There is no sensor.** A sensor \( H \) that is not 1 is part of
  \( P \). The loop the program shapes is \( L = PC \).

> The figure `Resources/Images/figures/especificaciones.tex` was written
> before this and says something else in two places: it puts \( F \) in all
> six, and it gives the sensor noise as \( |F/(1+PGH)| \), which is the
> sensitivity — the same magnitude it gives for the output disturbance, so
> two different requirements would mean the same thing. The thesis, the
> literature and the program agree against it.

## Where each one applies

A specification carries a frequency BAND and, inside it, the design
frequencies it is taken out of:

- the band is `min-frequency` to `max-frequency`, closed at both ends;
- `skipped` lists the design frequencies inside the band that this
  specification does NOT apply at.

`Specification::appliesAt(omega)` answers both, and everything else asks
that question and no other. The exception list is written this way round —
the frequencies that do not count, rather than the ones that do — so that a
design frequency added later falls inside the band and counts, instead of
quietly dropping out of a requirement.

The interface asks for this as one tick box per design frequency: the band
runs from the first ticked to the last, and what is unticked between them
becomes the exception list. A file written before the exceptions existed
carries none, which is the whole band.

## The bound

A bound is either a constant magnitude or a transfer function. Constants are
stored in LINEAR units and shown in decibels, which is the unit everything
downstream cuts at (`Specification::boundDb`). A transfer function is
evaluated at each frequency, and its magnitude in decibels is the bound
there.

Both are refused unless they make sense: a constant must be finite and
positive in linear units, a system bound must be a system, and the band must
satisfy `0 <= min <= max`. The form checks all three where they are typed,
and the `Specification` factories check them again before anything is
computed.
