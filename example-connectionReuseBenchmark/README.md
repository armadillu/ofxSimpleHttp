# Connection reuse download benchmark

Small console example comparing fresh Poco HTTP(S) sessions with opt-in reuse.
No window or GPU is needed. Uses the real ofxSimpleHttp code on the experimental
connection-reuse branch. It does not change the addon's default settings.

`bin/data/image-urls.txt` contains 40 unique HTTPS image URLs extracted from
`ofxApp/example/bin/data/testJson/ch_small.json`, choosing the `z` image size in
source order. They remain HTTPS: changing them to plain HTTP would remove the
TLS handshake cost being measured. The museum's supplied checksums are not used
in this transport-only benchmark; addon checksum behavior is unchanged.

From this example directory:

```sh
make Release -j"$(nproc)" JOBS="-j$(nproc)"
cd bin
./example-connectionReuseBenchmark
```

Optional positional arguments:

```text
./example-connectionReuseBenchmark [images=20] [workers=1] [rounds=2] [pause_ms=100] [url_list=data/image-urls.txt]
```

For example, compare ten images, one concurrent worker and two paired rounds:

```sh
./example-connectionReuseBenchmark 10 1 2 100
```

Each paired round downloads the same list twice: once with fresh sessions and
once with connection reuse. Order alternates between rounds (fresh/reuse, then
reuse/fresh). Each worker owns an independent downloader; the URLs are partitioned
identically in both modes. The pause applies between requests on each worker,
including in blocking mode. There are no application retries. All bodies are
transferred to RAM, avoiding disk caching or file-existence shortcuts.

Per-pass wall time and successful-payload MiB/s include connection setup and
between-request pauses. Per-request time includes network and downloader work,
but not the deliberate pause. The final speedup is reported only if every paired
pass succeeds with equal total payload sizes. A response must be HTTP 200 with
an image content type to count as successful. Failed or non-comparable runs exit
nonzero. `benchmark-results.csv` is overwritten on each run and contains every
request's mode, round, worker count, pause, HTTP status, byte count, duration and URL.

This is a small real-server comparison, not a guarantee: server/CDN caching,
network variation, protocol negotiation and concurrency all affect the result.
Keep concurrency and the pause reasonable for the host. Start with one worker
to make connection setup savings easy to see; then repeat at your real workload's
concurrency. The example follows the addon's current TLS context configuration.
