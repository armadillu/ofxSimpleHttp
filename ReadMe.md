# ofxSimpleHttp

[![Build Status](https://travis-ci.org/armadillu/ofxSimpleHttp.svg?branch=master)](https://travis-ci.org/armadillu/ofxSimpleHttp)
[![Build status](https://ci.appveyor.com/api/projects/status/d8e634rmni9f7tmr/branch/master?svg=true)](https://ci.appveyor.com/project/armadillu/ofxsimplehttp/branch/master)

OpenFrameworks addon to make http downloads easy; 


* supports http://, https:// and file://
* threaded downloads
* user agent customization
* proxy support
* custom timeouts
* parallel downloads
* download queues
* cancel downloads
* checksum checked downloads (sha1)
* skip download if file is already there and sha1 matches
* download reports (http status codes, download speed, total time, etc)
* ofEvents from main thread, or bg thread if required

![img1](https://farm6.staticflickr.com/5556/14785717148_c953b4d82e_z_d.jpg)

![img1](https://farm4.staticflickr.com/3909/14785770407_718d5752cf_z_d.jpg)

## Opt-in HTTP connection reuse

Connection reuse is **disabled by default**. Existing callers retain a fresh
HTTP(S) session per request. Checksums, retries, delays, concurrency, TLS
verification, notifications, and download policies are unchanged.

Enable reuse on an individual downloader before submitting requests:

```cpp
ofxSimpleHttp http;
http.setConnectionReuse(true);
```

The same opt-in is available on the orchestration classes:

```cpp
ofxBatchDownloader batch;
batch.setConnectionReuse(true); // before startDownloading()

ofxDownloadCentral central;
central.setConnectionReuse(true); // before downloadResources()
```

For download central, the setting applies to newly queued batches. It does not
modify already queued or active batches. Each batch has its own session; there
is no shared pool and no reuse across destroyed batch objects.

### Session lifetime

An instance caches at most one session. Requests to the same scheme, host, port,
proxy configuration, credentials, and HTTPS context can reuse it. A change in
those values discards the previous session. Timeouts are applied per request.
Only fully consumed, non-cancelled responses with valid transport lengths and
keep-alive permission return to the cache. Checksum failures still fail as
before; they do not make a fully consumed HTTP connection unsafe to reuse.

Poco manages keep-alive reconnects. If a stale connection nevertheless fails a
request, the session is discarded and the failure follows the existing caller
or batch retry path. This feature adds no hidden retries. Cancellation and
incomplete responses also discard the session.

Configure while idle, as with the addon's other settings. Cached-session access
is serialized per instance, but this does not make the existing mutable settings
or shared blocking-response API generally thread-safe. Do not mix blocking and
queued calls concurrently on one instance. Turning reuse off closes the cached
session; it does not interrupt an active transfer. Close/destroy downloader
instances (or turn reuse off while idle) before `destroySslContext()`.

### Validation

From the addon directory, on Linux with the normal OF build dependencies,
Python 3 and the OpenSSL command-line tool installed:

```sh
python3 tests/connection_reuse/run.py /path/to/OpenFrameworks
```

The script builds a temporary console project with all available CPUs and uses
local HTTP/1.1 and self-signed HTTPS servers. Connection IDs prove actual reuse.
Tests cover default-off behavior, opting in/out, chunked bodies, server-requested
closure, silently closed idle connections, origin/proxy/credential changes,
updated timeouts, truncation, disk downloads, unchanged checksum failure,
cancellation, queued callbacks, batch/central forwarding, and HTTPS reuse.
No external museum requests are made. The TLS fixture uses the addon's existing
context defaults; the feature does not change certificate verification policy.

Tested here on Linux with the installed Poco/OF versions. Other platforms and
Poco versions need validation before this opt-in should be deployed broadly.
