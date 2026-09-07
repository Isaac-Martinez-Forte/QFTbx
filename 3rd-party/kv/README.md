# kv, interval arithmetic

The interval arithmetic of the loop-shaping algorithms: `kv`, a C++ library for
verified numerical computation by Masahide Kashiwagi, MIT licence
(https://github.com/mskashi/kv, version 0.4.62). Only the headers the toolbox
needs are here: the real interval, its constants and the two rounding
back-ends. The build defines `KV_NOHWROUND`, so the directed roundings are
obtained with error-free transformations (twosum, twoproduct with fma) and the
floating-point rounding mode is never changed. The `boost/` directory holds
the two Boost utilities kv includes, written over the standard library, so
Boost need not be installed.

Everything else in the project uses `qftbx::Interval` and its companions from
`src/core/math/interval.h`, never kv directly.
