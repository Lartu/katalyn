# LSPL benchmarks

Run the small, dependency-free benchmark suite with:

```sh
make benchmark
```

The programs separately exercise core arithmetic and VM dispatch, function
calls and scopes, table reads and writes, and actor message round trips. Their
output is ignored; the reported `real` and `user` values are elapsed and CPU
seconds.

The older `legacy_replace.lspl` and `legacy_explode.lspl` workloads and their
Quijote text fixtures are retained here as additional manual benchmarks.

For useful comparisons, close CPU-intensive applications, run the suite several
times, and compare builds on the same computer. These are engineering
microbenchmarks, not claims about performance on other hardware or workloads.
