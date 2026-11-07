# Cooper Hewitt comparison — 7 November 2026

Built using all available CPUs. Ran from `bin/`:

```sh
./example-connectionReuseBenchmark 10 1 2 100
```

Ten unique HTTPS image URLs, one worker, two paired rounds, 100 ms pause
between requests. All 40 transfers returned HTTP 200 and image content types.
Each pass received 4,411,395 bytes; no application retries or checksum checks.

| Round | Order | Fresh sessions | Reused session |
|---|---|---:|---:|
| 1 | Fresh then reuse | 14.566 s | 4.889 s |
| 2 | Reuse then fresh | 13.448 s | 5.150 s |
| Combined | | 28.014 s | 10.039 s |

Observed wall-time ratio: **2.791x faster with reuse** in this sample.
This includes the identical deliberate pauses. It is not a guarantee for other
hosts, network conditions, payload sizes, or concurrency settings.

The local `bin/benchmark-results.csv` contains the per-request measurements and
is intentionally ignored by Git; rerunning overwrites it. Console output from
this run was captured in `/tmp/cooper-hewitt-download-benchmark.log`.
