# The `.qft` project file

A QFTbx project is saved as one XML file with the `.qft` extension. It holds
everything the design has produced so far: the plant, the design
frequencies, the specifications, the computed templates and their contours,
the boundaries and their union, the controller structure and the result of
the loop shaping. A project saved after any step reopens at that step.

The file is read and written by `src/persistence/` (pugixml), and the tag
names live in one table, `src/persistence/qft_dialect.h`, shared by the
reader and the writer so the two cannot drift apart.

## Version

The root element carries the format version:

    <?xml version="1.0" encoding="UTF-8"?>
    <QFT version="4">
      <inputs>   … </inputs>
      <settings> … </settings>
      <results>  … </results>
    </QFT>

This build writes and reads version 4, and refuses anything else rather
than guessing at it: the dialects this format has had share tag names with
DIFFERENT meanings - `<inicio>` is both a range start and an omega start -
so reading one as another would not fail, it would return wrong numbers.
The only .qft files that exist are the ones in this repository, and they
are at version 4.

## An unfinished project is a project

A .qft is saved at whatever point of the design it has reached, so any part
of it may be missing: a plant with no uncertainty entered, no design
frequencies, a specification section with three slots, a loop-shaping
section from a run that was interrupted. The reader takes what is there and
leaves what is not - that step simply stays undone, for the user to enter -
and says nothing about it, because there is nothing wrong with an
unfinished project.

Content that IS there and is broken is a different matter, and is refused
with the line it is on: a number that is not a number, a boolean that says
"perhaps", a list of complex values whose real and imaginary parts differ
in length. That is a damaged file, not an unfinished one.

## Structure

Three parts, in the order a reader meets them: **`<inputs>`** is what the
user described, **`<settings>`** what each computation was run with, and
**`<results>`** what came out of it, so that a file reads down the page: the
problem is legible in its first lines and the bulk of the numbers is at the
bottom.

The controller is an INPUT. It is the structure the search is asked to look
in - the zeros, the poles and the gain, each with the interval it may take -
and not what the search found, which is inside `<results>` under
`<loop-shaping>`. Before version 4 it sat after the templates and the
boundaries, which is what made the file hard to read.

A project saved early simply lacks the later sections, and one with nothing
computed has neither `<settings>` nor `<results>`.

### The inputs

**`<plant>`**, with a `name` attribute. Its `<type id="…">` says which of
the four forms the plant is written in (zero-pole-gain, time-constant,
polynomial, free-form expression in `s`). The form's parts follow:
`<expression>` for the free form, `<numerator>` and `<denominator>` with
their `size`, and the gain and the delay. Every uncertain or nominal
quantity is a `<parameter>`:

    <parameter>
      <nominal>5</nominal>
      <uncertain>true</uncertain>
      <name>a</name>
      <expr>a</expr>
      <range><min>1</min><max>5</max></range>
    </parameter>

`<name>` is the parameter's own name, `<expr>` its reparametrisation, an
expression of the toolbox's grammar (`a`, `a*10`, `2^b`), and `<range>`
the uncertainty interval. A parameter may take any identifier as a name
except the functions, the constants `pi` and `e` and the Laplace variable
`s`.

**`<omega>`**: the design frequencies, either generated (`<type>`, `<min>`,
`<max>`, `<point-count>`) or listed in `<values>`.

**`<specifications>`**: one `<specification>` per slot, with `<used>`, its
frequency range (`<min-frequency>`, `<max-frequency>`), whether it is
`<constant>`, and its `<magnitude>` or model.

**`<controller>`**: the controller structure the loop shaping searches, in
the same form and parameter syntax as the plant; the ranges are the search
box.

### The settings

**`<templates>`**: the `<epsilon>` the contour was walked with, one value
per frequency, and the plane those values are measured in:
`metric="nichols"` (degrees of phase and decibels of magnitude, the latter
divided by `db-per-degree`) or `metric="complex"` (the modulus of the
difference of two complex values). A file ported from version 2 is read as
`metric="complex"`.

**`<loop-shaping>`**: what the search was asked for, as three attributes.

    <loop-shaping algorithm="mc2" tolerance="0.05" columns="conservative"/>

`algorithm` is the name of one of `nt`, `nk`, `mr`, `mc1`, `mc-thesis`,
`mc2`, `mc3` - a name and not the position of the enumeration, so a file
still says what produced it after one more algorithm is added. `tolerance`
is the epsilon the algorithm stops at, which is the diameter of the Nichols
box for every algorithm but MR, where it is the width of the controller's
parameter box. `columns` is how a phase between two boundary nodes was
read: `nearest` takes the node it falls closest to, `conservative` takes
both, which is the reading that cannot return an infeasible design. The
same problem answers a different gain under each, so a design without these
three cannot be reproduced or compared.

### The results

**`<templates>`**: the `<full>` value sets and the `<contour>` of each
frequency, and the `<sweep>` they came from: one `<parameter name="a">` per
uncertain parameter with the grid of values it was swept over. The sweep is
what the verifier walks to close the loop with every member of the family;
a file without it still opens, and the verifier then says the family was
not checked.

**`<boundaries>`**: `<data>` with the Nichols grid (`<phases>` and
`<magnitudes>` with their `count`, `<min>` and `<max>`), the sheets as
`<open-flags>` and `<upper-flags>`, the `<columns>` of each specification
(per frequency and phase column, the count of allowed magnitude intervals
and then their ends, `inf` and `-inf` included), the `<per-frequency>`
boundaries and their `<union>` in `<union-buckets>`. A file without
`<columns>` is read all the same: they are rebuilt from the boundaries, a
cell coarser.

**`<loop-shaping>`**: the shaped controller and the plotted loop, with its
`point-count`, and the verifier's verdict on the design:

    <check satisfied="true" worst-excess-db="-1.25"
           family-members="625" family-unstable="0" family-worst-real-part="-0.65"/>

The worst excess over any active specification, in decibels, measured over
the full value sets and not over the boundaries the search worked against;
negative means satisfied and says by how much. The itemised table behind it
is not stored - it is derivable from what the file already carries - and
`worst-excess-db` is absent when no specification was active at any design
frequency. The three `family-*` attributes are the closed loop with every
plant of the sweep: how many plants, how many of them are closed-loop
unstable, and the largest real part of a closed-loop pole over all of them.
`satisfied` is true only when no specification is exceeded and no plant is
unstable. When the family could not be checked they are replaced by
`family-not-checked`, one of `no-sweep-record` (the templates came from a
file without `<sweep>`), `delay` (the loop has a delay, so its
characteristic equation is not a polynomial) or `not-rational`.

## Reading a file by hand

The fixtures in `tests/data/` are the reference: `planta1.qft` is a complete
project including loop shaping, and `qft_toolbox_ex2.qft` and `acc90.qft`
are the two thesis benchmarks with their templates and boundaries. A file
that fails to load reports the path, the line and what was expected; the
malformed files in the same folder show what is refused.
