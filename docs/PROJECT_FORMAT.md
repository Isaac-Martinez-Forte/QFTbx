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
    <QFT version="3">
      ...
    </QFT>

This build writes version 3 and reads versions 2 and 3. The only
difference is the plane the contour epsilon is measured in (see
`<templates>` below): a version 2 file has no such declaration and is read
as measuring in the complex plane, which is what those files meant.
Files from the earlier releases of QFTbx, which used Spanish tag names and
no version attribute, are refused with a message rather than guessed at;
they can be converted by opening and saving them with the last release that
read them. Every project shipped in `tests/data/` is version 2.

## Structure

The sections appear in pipeline order. A project saved early simply lacks
the later ones.

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

**`<templates>`**: `<metadata>` with the `<epsilon>` of the contour, the
`<full>` value sets and the `<contour>` of each frequency. Since version 3
the `<epsilon>` element carries the plane its values are measured in:
`metric="nichols"` (degrees of phase and decibels of magnitude, the latter
divided by `db-per-degree`) or `metric="complex"` (the modulus of the
difference of two complex values). A version 2 file is read as
`metric="complex"`.

**`<boundaries>`**: `<data>` with the Nichols grid (`<phases>` and
`<magnitudes>` with their `count`, `<min>` and `<max>`), the sheets as
`<open-flags>` and `<upper-flags>`, the `<columns>` of each specification
(per frequency and phase column, the count of allowed magnitude intervals
and then their ends, `inf` and `-inf` included), the `<per-frequency>`
boundaries and their `<union>` in `<union-buckets>`. A file without
`<columns>` is read all the same: they are rebuilt from the boundaries, a
cell coarser.

**`<controller>`**: the controller structure the loop shaping searches, in
the same form and parameter syntax as the plant; the ranges are the search
box.

**`<loop-shaping>`**: the shaped controller and the plotted loop, with its
`point-count`.

## Reading a file by hand

The fixtures in `tests/data/` are the reference: `planta1.qft` is a complete
project including loop shaping, and `qft_toolbox_ex2.qft` and `acc90.qft`
are the two thesis benchmarks with their templates and boundaries. A file
that fails to load reports the path, the line and what was expected; the
malformed files in the same folder show what is refused.
