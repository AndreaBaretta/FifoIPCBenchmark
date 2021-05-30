/*

Copyright © 2021 Andrea Baretta

This file is part of FifoIPCLatency.

FifoIPCLatency is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

FifoIPCLatency is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with FifoIPCLatency.  If not, see <https://www.gnu.org/licenses/>.

*/

#include <unistd.h>
#include <tuple>
#include <thread>

#include "fifo.hpp"
#include "benchmark.hpp"
#include "core_to_core.hpp"
#include "throughput_threads.hpp"
#include "cxxopts.hpp"


#ifdef NDEBUG
constexpr const bool test_mode = false;
#else
constexpr const bool test_mode = true;
#endif

#if defined(__x86_64__)
constexpr const bool use_avx256 = true;
#else
constexpr const bool use_avx256 = false;
#endif

constexpr const bool use_memcpy = !use_avx256;

using std::cout;
using std::endl;
using benchmark::fifo_t;
using benchmark::throughput_threads_t;
using namespace std::string_literals;

int main(int argc, char** argv) {
    cxxopts::Options options("ThreadToThreadThroughput", "Measures the latency of one thread communicating to another over a shared memory FIFO");
	options.add_options()
        ("ccx", "Number of CCXs per socket", cxxopts::value<int>()->default_value("1"))
        ("sockets", "Number of sockets in the system", cxxopts::value<int>()->default_value("1"))
        ("c,test-cache", "Tests the throughput of messages across L3 cache")
        ("i,test-interconnect", "Tests the throughput of messages across CCXs")
        ("s,test-sockets", "Tests throughput of messages across sockets")
        ("mt", "Perform this test with multithreading")
        ("h,help", "Print usage");

	auto result = options.parse(argc, argv);

    if (result.count("help")) {
    	cout << options.help() << endl;
        exit(0);
    }

	cout << "test_mode: " << test_mode << endl;
	cout << "use_avx256: " << use_avx256 << endl;

	const int num_ccx = result["ccx"].as<int>();
	const int num_sockets = result["sockets"].as<int>();
    const bool test_cache = result["test-cache"].as<bool>();
    const bool test_ccx = result["test-interconnect"].as<bool>();
    const bool test_sockets = result["test-sockets"].as<bool>();
    const bool mt = result["mt"].as<bool>();

    if (!test_cache && !test_ccx && !test_sockets) {
        cout << "No test selected" << endl;
        exit(0);
    }

	int max_cores = sysconf(_SC_NPROCESSORS_ONLN)/2;
	cout << "Number of cores detected: " << max_cores << endl;
    cout << "Number of CCXs: " << num_ccx << endl;
    cout << "Number of sockets: " << num_sockets << endl;

    const int ccx_size = max_cores / num_ccx;
    const int socket_size = max_cores / num_sockets;



    if (max_cores % num_sockets != 0 || max_cores / num_sockets % num_ccx != 0) {
        cout << "Bad number of sockets or CCXs" << endl;
        exit(0);
    }

    if (ccx_size % 2 != 0) {
        cout << "Odd number of cores per CCX" << endl;
        exit(0);
    }

    if (num_ccx % 2 != 0 && num_ccx != 1) {
        cout << "Odd number of CCXs per socket" << endl;
        exit(0);
    }

    if (num_sockets % 2 != 0 && num_sockets != 1) {
        cout << "Odd number of sockets" << endl;
        exit(0);
    }

    std::vector<std::tuple<int, int>> core_pairs{};
    
    if (test_cache) {
        cout << "Testing L3 cache throughput" << endl;
        for (int c = 0; c < ccx_size; c += 2) {
            const int core_1 = c;
            const int core_2 = c + 1;
            core_pairs.push_back({core_1, core_2});
            cout << "Assigned cores: " << core_1 << ", " << core_2 << endl;
        }
    } else if (test_ccx) {
        cout << "Testing interconnect throughput" << endl;
        for (int ccx = 0; ccx < num_ccx; ccx += 2) {
            for (int c = 0; c < ccx_size; ++c) {
                const int core_1 = ccx*ccx_size + c;
                const int core_2 = (ccx+1)*ccx_size + c;
                core_pairs.push_back({core_1, core_2});
                cout << "Assigned cores: " << core_1 << ", " << core_2 << endl;
            }
        }
    } else {
        cout << "Testing socket throughput" << endl;
        for (int socket = 0; socket < num_sockets; socket += 2) {
            for (int ccx = 0; ccx < num_ccx; ccx += 2) {
                for (int c = 0; c < ccx_size; ++c) {
                    const int core_1 = socket*socket_size + ccx*ccx_size + c;
                    const int core_2 = (socket+1)*socket_size + ccx*ccx_size + c;
                    core_pairs.push_back({core_1, core_2});
                    cout << "Assigned cores: " << core_1 << ", " << core_2 << endl;
                }
            }
        }
    }

    const int num_pairs_ = core_pairs.size();
    if (mt) {
        for (int i = 0; i < num_pairs_; ++i) {
            const int core_1 = std::get<0>(core_pairs[i]) + max_cores;
            const int core_2 = std::get<1>(core_pairs[i]) + max_cores;
            core_pairs.push_back({core_1, core_2});
            cout << "Assigned cores: " << core_1 << ", " << core_2 << endl;
        }
    }
    const int num_pairs = core_pairs.size();

    for (int i = 0; i < num_pairs; ++i) {
        const int core_1 = std::get<0>(core_pairs[i]);
        const int core_2 = std::get<1>(core_pairs[i]);
        cout << "i=" << i << ", Cores: " << core_1 << ", " << core_2 << endl;
    }

    std::vector<std::thread> threads{};
    std::atomic<bool> start = false;
    std::vector<throughput_threads_t<64, test_mode, use_memcpy, use_avx256>*> tts{};
    threads.reserve(num_pairs*2);
    tts.reserve(num_pairs);
    for (int i = 0; i < num_pairs; ++i) { //Create all thread objects
        const int core_1 = std::get<0>(core_pairs[i]);
        const int core_2 = std::get<1>(core_pairs[i]);
        cout << "Cores: " << core_1 << ", " << core_2 << endl;
        // benchmark::core_to_core_t<64, test_mode, use_memcpy, use_avx256> tt{core_1, core_2, 1000000, 8, 1, true}; //This is 3x slower
        auto* tt = new throughput_threads_t<64, test_mode, use_memcpy, use_avx256>{core_1, core_2, 1000000, 8, 1, true, start};
        // tts.emplace_back(core_1, core_2, 10, 8, 1, true, start);
        // cout << "Thread cores: " << tts[i].core_1 << ", " << tts[i].core_2 << endl;
        //std::thread thread_1([&]{tt.thread_1();});
        //std::thread thread_2([&]{tt.thread_2();});
        // auto ptr = new throughput_threads_t<64, test_mode, use_memcpy, use_avx256>{core_1, core_2, 1000000, 8, 1, true};
        // threads.emplace_back(ptr);
        // thread_1.join();
        // thread_2.join();
        // cout << "Threads: " << core_1 << ", " << core_2 << ": " << tt.thread_1.avg_latency << " ns" << endl;
        //std::thread* thread_1 = new std::thread([&]{tt.thread_1();});
        //std::thread* thread_2 = new std::thread([&]{tt.thread_2();});
        //threads.emplace_back(thread_1);
        //hreads.emplace_back(thread_2);
        threads.emplace_back([&]{tt->thread_1();});
        threads.emplace_back([&]{tt->thread_2();});
        cout << "Placed thread: i=" << i << endl;
        tts.emplace_back(tt);
    }

    long t1 = benchmark::get_thread_time_nano();
    start = true;

    for (std::thread& thread : threads) {
        thread.join();
    }
    long t = benchmark::get_thread_time_nano() - t1;
    cout << "Total time: " << t/1000000 << " sec" << endl;

    cout << "Done!" << endl;
	return 0;
}