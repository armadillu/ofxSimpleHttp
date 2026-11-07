#include "ofMain.h"
#include "ofxSimpleHttp.h"
#include "ofxBatchDownloader.h"
#include "ofxDownloadCentral.h"
#include <stdexcept>
#include <thread>
#include <chrono>
#include <cstdio>

class ReuseTestHttp : public ofxSimpleHttp {
public:
    // The blocking API retains its result between calls. Isolate test request
    // state like the normal async API, without changing production behavior.
    void clearBlockingResult(){ response = ofxSimpleHttpResponse(); }
};

static void check(bool condition, const std::string& description){
    if(!condition) throw std::runtime_error(description);
    std::cout << "PASS " << description << std::endl;
}
static std::string get(ReuseTestHttp& http, const std::string& url){
    http.clearBlockingResult();
    auto r = http.fetchURLBlocking(url);
    check(r.ok && r.status == 200, "GET " + url);
    return r.responseBody;
}
struct QueueResults {
    std::vector<std::string> bodies;
    int failures = 0;
    void received(ofxSimpleHttpResponse& r){
        if(r.ok) bodies.push_back(r.responseBody); else ++failures;
    }
};
struct BatchResults {
    int completed = 0;
    void received(ofxBatchDownloaderReport&){ ++completed; }
};

int main(int argc, char** argv){
    if(argc != 4) return 2;
    ofInit();
    const std::string base = argv[1], other = argv[2], tls = argv[3];
    try{
        ReuseTestHttp http;
        http.setSilent(true);
        http.setTimeOut(2);
        auto first = get(http, base + "/id");
        check(first != get(http, base + "/id"), "default opens separate connections");
        http.setConnectionReuse(true);
        first = get(http, base + "/id");
        check(first == get(http, base + "/id"), "opt-in reuses a connection");
        check(first == get(http, base + "/chunked"), "chunked response uses the same connection");
        check(first == get(http, base + "/id"), "chunked body is fully consumed before reuse");
        auto proxy = ofxSimpleHttp::ProxyConfig();
        auto colon = other.find_last_of(':');
        proxy.useProxy = true;
        proxy.host = "127.0.0.1";
        proxy.port = std::stoi(other.substr(colon + 1));
        http.setProxyConfiguration(proxy);
        auto proxied = get(http, base + "/id");
        check(proxied != first, "proxy configuration replaces the direct session");
        check(proxied == get(http, base + "/id"), "proxied requests reuse the same session");
        proxy.password = "new-proxy-password";
        proxy.login = "proxy-user";
        http.setProxyConfiguration(proxy);
        check(proxied != get(http, base + "/id"), "proxy credentials replace the cached session");
        proxy.useProxy = false;
        http.setProxyConfiguration(proxy);
        auto closed = get(http, base + "/close");
        check(closed != get(http, base + "/id"), "Connection close reconnects");
        auto idle = get(http, base + "/idle");
        std::this_thread::sleep_for(std::chrono::milliseconds(250));
        // A stale peer may fail the next GET; existing caller retry semantics apply.
        auto stale = http.fetchURLBlocking(base + "/id");
        if(!stale.ok) stale = http.fetchURLBlocking(base + "/id");
        check(stale.ok && stale.responseBody != idle, "silently closed idle connection recovers");
        first = get(http, base + "/id");
        get(http, other + "/id");
        check(first != get(http, base + "/id"), "origin switch discards old session");
        first = get(http, base + "/id");
        http.setCredentials("user", "password");
        check(first != get(http, base + "/id"), "credential change discards old session");
        first = get(http, base + "/id");
        http.setTimeOut(3);
        check(first == get(http, base + "/id"), "timeout update works with reused session");
        auto bad = http.fetchURLBlocking(base + "/truncate");
        check(!bad.ok, "truncated response fails");
        get(http, base + "/id");
        http.setConnectionReuse(false);
        first = get(http, base + "/id");
        check(first != get(http, base + "/id"), "opting out restores separate connections");
        http.setConnectionReuse(true);
        http.setNeedsChecksumMatchToSkipDownload(true);
        const std::string dir = ofToDataPath("single-downloads", true);
        auto disk = http.fetchURLtoDiskBlocking(base + "/id", dir);
        check(disk.ok && ofFile(disk.absolutePath).getSize() > 0, "download to disk succeeds");
        first = ofBufferFromFile(disk.absolutePath).getText();
        check(first == get(http, base + "/id"), "disk download retains reusable session");
        auto checksum = http.fetchURLtoDiskBlocking(base + "/id", dir, "intentionally-wrong-checksum");
        check(!checksum.ok && !checksum.checksumOK, "checksum mismatch behavior is unchanged");
        check(first == get(http, base + "/id"), "checksum mismatch does not corrupt transport");
        // Async cancellation must not retain a partially consumed response.
        first = get(http, base + "/id");
        http.setNotifyFromMainThread(false);
        http.fetchURL(base + "/slow", true);
        std::this_thread::sleep_for(std::chrono::milliseconds(120));
        http.stopCurrentDownload(true);
        http.waitForThread(false);
        check(first != get(http, base + "/id"), "async cancellation discards the incomplete connection");
        http.setConnectionReuse(false);
        {
            ofxSimpleHttp queued;
            queued.setSilent(true);
            queued.setConnectionReuse(true);
            QueueResults results;
            ofAddListener(queued.httpResponse, &results, &QueueResults::received);
            queued.fetchURL(base + "/id", true);
            queued.fetchURL(base + "/id", true);
            auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(5);
            while(results.bodies.size() < 2 && std::chrono::steady_clock::now() < deadline){
                queued.update();
                std::this_thread::sleep_for(std::chrono::milliseconds(5));
            }
            queued.waitForThread(false);
            queued.update();
            ofRemoveListener(queued.httpResponse, &results, &QueueResults::received);
            check(results.failures == 0 && results.bodies.size() == 2 &&
                  results.bodies[0] == results.bodies[1], "async queue reuses a connection and delivers callbacks");
        }
        {
            ofxBatchDownloader batch;
            batch.setConnectionReuse(true);
            batch.setDownloadFolder("batch-downloads");
            batch.setNeedsChecksumMatchToSkipDownload(true);
            batch.setTimeOut(2);
            batch.setMaxRetries(0);
            batch.addResourcesToDownloadList({base + "/batch-one", base + "/batch-two"});
            batch.startDownloading();
            auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(5);
            while(batch.isBusy() && std::chrono::steady_clock::now() < deadline){
                batch.update();
                std::this_thread::sleep_for(std::chrono::milliseconds(5));
            }
            check(!batch.isBusy() && ofFile::doesFileExist("batch-downloads/batch-one") &&
                  ofBufferFromFile("batch-downloads/batch-one").getText() ==
                  ofBufferFromFile("batch-downloads/batch-two").getText(), "batch downloader forwards opt-in and reuses a connection");
        }
        {
            ofxDownloadCentral central;
            central.setConnectionReuse(true);
            central.setMaxConcurrentDownloads(2);
            central.setTimeOut(2);
            central.setMaxRetries(0);
            BatchResults results;
            central.downloadResources(std::vector<std::string>{base + "/central-one", base + "/central-two"},
                                      &results, &BatchResults::received, "central-downloads");
            central.startDownloading();
            auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(5);
            while(central.isBusy() && std::chrono::steady_clock::now() < deadline){
                central.update();
                std::this_thread::sleep_for(std::chrono::milliseconds(5));
            }
            check(!central.isBusy() && results.completed == 1 &&
                  ofFile::doesFileExist("central-downloads/central-one") &&
                  ofBufferFromFile("central-downloads/central-one").getText() ==
                  ofBufferFromFile("central-downloads/central-two").getText(), "download central forwards opt-in to newly queued batches");
        }
        ofxSimpleHttp::createSslContext();
        {
            ReuseTestHttp https;
            https.setSilent(true);
            https.setConnectionReuse(true);
            first = get(https, tls + "/id");
            check(first == get(https, tls + "/id"), "HTTPS session is reused");
            get(https, base + "/id");
            check(first != get(https, tls + "/id"), "HTTP/HTTPS switch creates a new session");
        }
        ofxSimpleHttp::destroySslContext();
        std::cout << "ALL CONNECTION REUSE TESTS PASSED" << std::endl;
        return 0;
    }catch(const std::exception& e){
        std::cerr << "FAIL " << e.what() << std::endl;
        return 1;
    }
}
