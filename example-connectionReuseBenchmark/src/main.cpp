#include "ofMain.h"
#include "ofxSimpleHttp.h"
#include <algorithm>
#include <chrono>
#include <fstream>
#include <iomanip>
#include <mutex>
#include <thread>

using Clock = std::chrono::steady_clock;
struct Result {
    std::string url;
    int status = 0;
    bool ok = false;
    std::size_t bytes = 0;
    double seconds = 0;
};
struct Summary { double seconds; std::size_t bytes; int successes; };

static Summary run(const std::vector<std::string>& urls, bool reuse, int workers,
                   int pauseMs, int round, std::ofstream& csv){
    std::vector<Result> results(urls.size());
    std::vector<std::thread> threads;
    std::mutex output;
    auto start = Clock::now();
    for(int worker = 0; worker < workers; ++worker){
        threads.emplace_back([&, worker]{
            ofxSimpleHttp http;
            http.setSilent(true);
            http.setTimeOut(20);
            http.setConnectionReuse(reuse);
            for(std::size_t i = worker; i < urls.size(); i += workers){
                auto before = Clock::now();
                auto response = http.fetchURLBlocking(urls[i]);
                Result& r = results[i];
                r.url = urls[i];
                r.status = response.status;
                r.ok = response.ok && response.status == 200 &&
                       response.contentType.find("image/") == 0;
                r.bytes = response.responseBody.size();
                r.seconds = std::chrono::duration<double>(Clock::now() - before).count();
                {
                    std::lock_guard<std::mutex> guard(output);
                    std::cout << (reuse ? "reuse" : "fresh") << " " << i+1 << "/" << urls.size()
                              << " HTTP " << r.status << " " << r.bytes << " bytes "
                              << std::fixed << std::setprecision(3) << r.seconds << " s"
                              << (r.ok ? "" : " FAILED") << std::endl;
                }
                if(i + workers < urls.size())
                    std::this_thread::sleep_for(std::chrono::milliseconds(pauseMs));
            }
        });
    }
    for(auto& thread : threads) thread.join();
    Summary s{std::chrono::duration<double>(Clock::now() - start).count(), 0, 0};
    for(std::size_t i = 0; i < results.size(); ++i){
        const auto& r = results[i];
        if(r.ok){ s.bytes += r.bytes; ++s.successes; }
        csv << round << ',' << (reuse ? "reuse" : "fresh") << ',' << workers << ','
            << pauseMs << ',' << i+1 << ',' << r.status << ',' << r.ok << ',' << r.bytes
            << ',' << r.seconds << ',' << '"' << r.url << '"' << '\n';
    }
    csv.flush();
    std::cout << "SUMMARY round=" << round << " mode=" << (reuse ? "reuse" : "fresh")
              << " success=" << s.successes << '/' << urls.size()
              << " wall=" << s.seconds << " s throughput="
              << (s.bytes / (1024.0*1024.0) / s.seconds) << " MiB/s\n" << std::endl;
    return s;
}

int main(int argc, char** argv){
    // No window or renderer is required; responses are downloaded to RAM.
    ofInit();
    try{
        int limit = argc > 1 ? std::stoi(argv[1]) : 20;
        int workers = argc > 2 ? std::stoi(argv[2]) : 1;
        int rounds = argc > 3 ? std::stoi(argv[3]) : 2;
        int pauseMs = argc > 4 ? std::stoi(argv[4]) : 100;
        std::string listPath = argc > 5 ? argv[5] : "data/image-urls.txt";
        if(limit < 1 || workers < 1 || rounds < 1 || pauseMs < 0)
            throw std::runtime_error("count, workers, rounds must be positive; pause must be nonnegative");
        std::ifstream list(listPath);
        if(!list) throw std::runtime_error("Cannot open URL list: " + listPath + "; run from bin/");
        std::vector<std::string> urls;
        for(std::string line; std::getline(list, line) && int(urls.size()) < limit;){
            if(!line.empty() && line.back() == '\r') line.pop_back();
            if(line.empty() || line[0] == '#') continue;
            if(line.find("https://") != 0 && line.find("http://") != 0)
                throw std::runtime_error("URL must use HTTP or HTTPS");
            urls.push_back(line);
        }
        if(urls.empty()) throw std::runtime_error("URL list is empty");
        workers = std::min(workers, int(urls.size()));
        std::ofstream csv("benchmark-results.csv");
        if(!csv) throw std::runtime_error("Cannot write benchmark-results.csv");
        csv << "round,mode,workers,pause_ms,index,http_status,ok,bytes,seconds,url\n";
        ofxSimpleHttp::createSslContext();
        double freshTime = 0, reusedTime = 0;
        bool allPassed = true;
        std::cout << urls.size() << " images; " << workers << " workers; " << pauseMs
                  << " ms pause per worker; " << rounds << " paired rounds\n";
        for(int round = 1; round <= rounds; ++round){
            // Alternate order to reduce bias from server/CDN warm-up.
            Summary fresh{}, reused{};
            if(round % 2){
                fresh = run(urls, false, workers, pauseMs, round, csv);
                reused = run(urls, true, workers, pauseMs, round, csv);
            }else{
                reused = run(urls, true, workers, pauseMs, round, csv);
                fresh = run(urls, false, workers, pauseMs, round, csv);
            }
            freshTime += fresh.seconds; reusedTime += reused.seconds;
            bool comparable = fresh.successes == int(urls.size()) &&
                              reused.successes == int(urls.size()) && fresh.bytes == reused.bytes;
            allPassed = allPassed && comparable;
            if(!comparable) std::cout << "Round not comparable: failures or different byte totals\n";
        }
        ofxSimpleHttp::destroySslContext();
        if(allPassed){
            std::cout << "Fresh total: " << freshTime << " s; reuse total: " << reusedTime
                      << " s; observed speedup: " << freshTime/reusedTime << "x\n";
        }else std::cout << "No overall speedup reported: not all paired transfers were comparable\n";
        std::cout << "Per-request timings saved to benchmark-results.csv\n";
        return allPassed ? 0 : 1;
    }catch(const std::exception& e){
        std::cerr << e.what() << "\nUsage: benchmark [images=20] [workers=1] [rounds=2] [pause_ms=100] [url_list=data/image-urls.txt]\n";
        return 2;
    }
}
