/*

Copyright © 2021 Andrea Baretta

This file is part of FifoIPCBenchmark.

FifoIPCBenchmark is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

FifoIPCBenchmark is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with FifoIPCBenchmark. If not, see <https://www.gnu.org/licenses/>.

*/

#include <unistd.h>
#include <tuple>
#include <thread>
#include <filesystem>
#include <iostream>
#include <fstream>

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

int max_cores = sysconf(_SC_NPROCESSORS_ONLN)/2;
// int max_cores = 64;

int rotate_in_ccx(int core, int r, int ccx_size) {
    return (core + r)%ccx_size + (core/ccx_size)*ccx_size; //I don't know why but this is funny to me
}

std::string status_of_core(std::vector<std::tuple<int, int>>& core_pairs, int core) {
    for (std::tuple<int, int> pair : core_pairs) {
        if (std::get<0>(pair) == core) {
            return "TX"s;
        } else if (std::get<1>(pair) == core) {
            return "RX"s;
        }
    }
    return "-"s;
}

void run_test(std::vector<std::tuple<int, int>>& core_pairs, const int num_tries, const std::size_t fifo_size, const std::size_t message_size, 
              std::vector<std::thread>& threads, std::filesystem::path file, std::ofstream& data, std::string test_name) {
    const int num_pairs = core_pairs.size();
    std::vector<throughput_threads_t<64, test_mode, use_memcpy, use_avx256>*> tts{};
    threads.reserve(num_pairs*2);
    tts.reserve(num_pairs);

    std::atomic<bool> start = false;

    for (int i = 0; i < num_pairs; ++i) { //Create all thread objects
        const int core_1 = std::get<0>(core_pairs[i]);
        const int core_2 = std::get<1>(core_pairs[i]);
        cout << "Cores: " << core_1 << ", " << core_2 << endl;
        auto* tt = new throughput_threads_t<64, test_mode, use_memcpy, use_avx256>{core_1, core_2, num_tries, fifo_size, message_size, start};
        threads.emplace_back([&]{tt->thread_1();});
        threads.emplace_back([&]{tt->thread_2();});
        cout << "Placed thread: i=" << i << endl;
        tts.emplace_back(tt);
    }
    start = true;
    for (std::thread& thread : threads) {
        thread.join();
    }
    
    for (int i = 0; i < num_pairs; ++i) {
        data << test_name << "," << num_pairs << "," << std::get<0>(core_pairs[i]) << "," << tts[i]->thread_1.avg_throughput << ",";
        for (int c = 0; c < max_cores*2; ++c) {
            data << status_of_core(core_pairs, c) << ",";
        }
        data << "\n"s;
    }
    for (auto* ptr : tts) { delete ptr; };
}

int main(int argc, char** argv) {
    cxxopts::Options options("ThreadToThreadThroughput", "Measures the latency of one thread communicating to another over a shared memory FIFO");
	options.add_options()
        ("ccx", "Number of CCXs per socket", cxxopts::value<int>()->default_value("1"))
        ("sockets", "Number of sockets in the system", cxxopts::value<int>()->default_value("1"))
        ("intra-core", "Tests the throughput of message across two threads in the same physical core")
        ("intra-ccx", "Tests the throughput of messages across pairs of cores in the same CCX")
        ("intra-ccx-all", "Tests the throughput of messages across all pairs of cores in one CCX on all CCXs at the same time")
        ("inter-ccx", "Tests communication across all pairs of CCXs in one socket")
        ("inter-socket", "Tests the throughput of messages across all pairs of cores between sockets")
        ("mt", "Perform this test with simultaneous multithreading")
        ("b,buffer-size", "Size of the buffer in the shared FIFO in number of messages", cxxopts::value<std::size_t>()->default_value("8"))
        ("m,message-size", "Size of the message sent to FIFO in number of cache lines", cxxopts::value<std::size_t>()->default_value("1"))
        ("n,numtries", "Number of iterations of the test", cxxopts::value<int>()->default_value("1000000"))
        ("d,data-dir", "Directory for gathered data. If not specified, uses present working directory", cxxopts::value<std::string>()->default_value(std::string("")))
        ("h,help", "Print usage");

	auto result = options.parse(argc, argv);

    if (result.count("help")) {
    	cout << options.help() << endl;
        exit(0);
    }

	cout << "test_mode: " << test_mode << endl;
	cout << "use_avx256: " << use_avx256 << endl;

	const int num_sockets = result["sockets"].as<int>();
	const int num_ccx_per_socket = result["ccx"].as<int>();

    const bool intra_core = result["intra-core"].as<bool>();
    const bool intra_ccx = result["intra-ccx"].as<bool>();
    const bool intra_ccx_all = result["intra-ccx-all"].as<bool>();
    const bool inter_ccx = result["inter-ccx"].as<bool>();
    const bool inter_socket = result["inter-socket"].as<bool>();

    const std::size_t fifo_size = result["buffer-size"].as<std::size_t>();
    const std::size_t message_size = result["message-size"].as<std::size_t>();
    const int num_tries = result["numtries"].as<int>();
    std::string usr_data_dir = result["data-dir"].as<std::string>();

    const bool mt = result["mt"].as<bool>();

	cout << "Number of cores detected: " << max_cores << endl;
    cout << "Number of CCXs per socket: " << num_ccx_per_socket << endl;
    cout << "Number of sockets: " << num_sockets << endl;

    const int ccx_size = max_cores / (num_ccx_per_socket*num_sockets);
    const int socket_size = max_cores / num_sockets;

    if (num_sockets % 2 != 0 && num_sockets != 1) {
        cout << "ERROR: Odd number of sockets" << endl;
        exit(0);
    }

    if (num_ccx_per_socket % 2 != 0 && inter_ccx) {
        cout << "ERROR: Running inter-CCX test with odd number of CCXs per socket" << endl;
        exit(0);
    }

    if (max_cores % (num_ccx_per_socket*num_sockets) != 0) {
        cout << "ERROR: Odd number of cores per CCX" << endl;
        exit(0);
    }

    std::vector<std::vector<std::tuple<int, int>>> tests{};
    
    if (intra_core) {
        cout << "Testing L1/L2 cache throughput" << endl;
        for (int i = 0; i < ccx_size; ++i) {
            tests.push_back(std::vector{std::tuple{0 + i,max_cores + i}});
        }
    } else if (intra_ccx) {
        cout << "Testing L3 cache throughput on one CCX" << endl;
        std::vector<std::vector<std::tuple<int, int>>> core_pairs{};
        for (int i = 0; i < ccx_size; ++i) { core_pairs.push_back(std::vector<std::tuple<int, int>>{}); }
        for (int c = 0; c < ccx_size; c += 2) {
            const int core_1 = c;
            const int core_2 = c + 1;
            for (int i = 0; i < ccx_size; ++i) {
                core_pairs[i].push_back({rotate_in_ccx(core_1, i, ccx_size), rotate_in_ccx(core_2, i, ccx_size)});
                tests.push_back(core_pairs[i]);
            }
            cout << "Assigned cores: " << core_1 << ", " << core_2 << endl;
        }
    } else if (intra_ccx_all) {
        cout << "Testing L3 cache throughput on all CCXs" << endl;
        for (int i = 0; i < ccx_size; ++i) {
            std::vector<std::tuple<int, int>> core_pairs{};
            for (int ccx = 0; ccx < num_ccx_per_socket; ++ccx) {
                for (int c = 0; c < ccx_size; c += 2) {
                    const int core_1 = rotate_in_ccx(ccx*ccx_size + c, i, ccx_size);
                    const int core_2 = rotate_in_ccx(ccx*ccx_size + c + 1, i, ccx_size);
                    core_pairs.push_back({core_1, core_2});
                    cout << "Assigned cores: " << core_1 << ", " << core_2 << endl;
                }
            }
            tests.push_back(core_pairs);
        }
    } else if (inter_ccx) {
        cout << "Testing interconnect throughput" << endl;
        if (num_ccx_per_socket == 1) {
            cout << "Only one CCX" << endl;
            exit(0);
        }
        std::vector<std::vector<std::tuple<int, int>>> core_pairs{};
        for (int i = 0; i < ccx_size; ++i) { core_pairs.push_back(std::vector<std::tuple<int, int>>{}); }
        for (int ccx = 0; ccx < num_ccx_per_socket; ccx += 2) {
            for (int c = 0; c < ccx_size; ++c) {
                const int core_1 = ccx*ccx_size + c;
                const int core_2 = (ccx+1)*ccx_size + c;
                for (int i = 0; i < ccx_size; ++i) {
                    core_pairs[i].push_back({rotate_in_ccx(core_1, i, ccx_size), rotate_in_ccx(core_2, i, ccx_size)});
                    tests.push_back(core_pairs[i]);
                }
                cout << "Assigned cores: " << core_1 << ", " << core_2 << endl;
            }
        }
    } else if (inter_socket) {
        cout << "Testing socket throughput" << endl;
        if (num_sockets == 1) {
            cout << "Only one socket" << endl;
            exit(0);
        }
        std::vector<std::vector<std::tuple<int, int>>> core_pairs{};
        for (int i = 0; i < ccx_size; ++i) { core_pairs.push_back(std::vector<std::tuple<int, int>>{}); }
        for (int socket = 0; socket < num_sockets; socket += 2) {
            for (int ccx = 0; ccx < num_ccx_per_socket; ++ccx) {
                for (int c = 0; c < ccx_size; ++c) {
                    const int core_1 = socket*socket_size + ccx*ccx_size + c;
                    const int core_2 = (socket+1)*socket_size + ccx*ccx_size + c;
                    for (int i = 0; i < ccx_size; ++i) {
                        core_pairs[i].push_back({rotate_in_ccx(core_1, i, ccx_size), rotate_in_ccx(core_2, i, ccx_size)});
                        tests.push_back(core_pairs[i]);
                    }
                    cout << "Assigned cores: " << core_1 << ", " << core_2 << endl;
                }
            }
        }
    } else {
        cout << "No test selected" << endl;
        exit(0);
    }

    const int num_tests = tests.size();
    if (mt && !intra_core) {
        for (int t = 0; t < num_tests; ++t) {
            int num_pairs = tests[t].size();
            for (int i = 0; i < num_pairs; ++i) {
                const int core_1 = std::get<0>(tests[t][i]) + max_cores;
                const int core_2 = std::get<1>(tests[t][i]) + max_cores;
                tests[i].push_back({core_1, core_2});
                cout << "Assigned cores: " << core_1 << ", " << core_2 << endl;
            }
        }
    }

    for (int t = 0; t < tests.size(); ++t) {
        cout << "Test #" << t << ": size = " << tests[t].size() << endl;
        for (int i = 0; i < tests[t].size(); ++i) {
            const int core_1 = std::get<0>(tests[t][i]);
            const int core_2 = std::get<1>(tests[t][i]);
            cout << "Cores: " << core_1 << ", " << core_2 << endl;
        }
    }

	std::filesystem::path data_dir =
			usr_data_dir.empty()
			? std::filesystem::current_path() / "data"
			: std::filesystem::path(std::string(usr_data_dir));
    std::filesystem::create_directory(data_dir);

    std::string sysinfo_file_name = data_dir.string() + "/sysinfo.txt"s;
	if (!std::filesystem::exists(sysinfo_file_name)) {
		std::string cpu_cmd = "lscpu | head -n 26 > "s + sysinfo_file_name;
		std::string memamount_cmd = "grep MemTotal /proc/meminfo >> "s + sysinfo_file_name;
		std::string memspeed_cmd = "sudo dmidecode --type 17| grep Speed >> "s + sysinfo_file_name;
		std::string os_cmd = "head -n 6 /etc/os-release >> "s + sysinfo_file_name;
		std::string kernel_cmd = "echo -n \"Kernel version: \" >> "s + sysinfo_file_name + "; uname -r >> "s + sysinfo_file_name;

		if (system(cpu_cmd.c_str()) != 0) {
			cout << "Couldn't read cpu info (lscpu)" << endl;
		}
		if (system(memamount_cmd.c_str()) != 0) {
			cout << "Couldn't read mem amount (/proc/meminfo)" << endl;
		}
		if (system(memspeed_cmd.c_str()) != 0) {
			cout << "Couldn't read mem speed (dmidecode)" << endl;
		}
		if (system(os_cmd.c_str()) != 0) {
			cout << "Couldn't read OS info (/etc/os-release)" << endl;
		}
		if (system(kernel_cmd.c_str()) != 0) {
			cout << "Couldn't read kernel info (uname -r)" << endl;
		}
	}

    std::string test_name;
    if (intra_core) { test_name = "intra_core"s; }
    else if (intra_ccx) { test_name = "intra_ccx"s; }
    else if (intra_ccx_all) { test_name = "intra_ccx_all"s; }
    else if (inter_ccx) { test_name = "inter_ccx"s; }
    else { test_name = "inter_socket"s; }
    if (mt) { test_name += "_mt"s; }

    data_dir = data_dir / (test_name + ".csv"s);
    std::ofstream data;
    data.open(data_dir, std::ios_base::trunc);
    data << "test_type,num_pairs,thread_id,result,";
    for (int c = 0; c < max_cores*2; ++c) {
        data << c << ",";
    }
    data << "\n"s;
    for (int t = 0; t < tests.size(); ++t) {
        cout << "Beginning test #" << t << endl;
        std::vector<std::thread> threads{};
        run_test(tests[t], num_tries, fifo_size, message_size, threads, data_dir, data, test_name);
    }
    data.close();

    cout << "Done!" << endl;
	return 0;
}