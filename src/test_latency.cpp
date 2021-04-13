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

#include <iostream>
#include <fstream>
#include <thread>
#include <stdio.h>
#include <unistd.h>
#include <limits.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <string>
#include <filesystem>
#include <assert.h>

#include "fifo.hpp"
#include "benchmark.hpp"
#include "core_to_core.hpp"
#include "cxxopts.hpp"


#ifdef NDEBUG
constexpr const bool test_mode = false;
#else
constexpr const bool test_mode = true;
#endif

#ifdef NATIVE
constexpr const bool use_avx256 = true;
#else
constexpr const bool use_avx256 = false;
#endif

constexpr const bool use_memcpy = !use_avx256;

using std::cout;
using std::endl;
using benchmark::fifo_t;
using benchmark::core_to_core_t;
using namespace std::string_literals;

int main(int argc, char** argv) {
	cxxopts::Options options("ThreadToThreadLatency", "Measures the latency of one thread communicating to another over a shared memory FIFO");
	options.add_options()
			("n,numtries", "Number of iterations of the test", cxxopts::value<int>()->default_value("1000000"))
			("i,individual-message", "Save latency data on individual messages")
			("d,data-dir", "Directory for gathered data. If used with -i, it will save latency data on individual messages. If not specified, uses present working directory", cxxopts::value<std::string>()->default_value(std::string("")))
			("cores", "If not specified, tests all cores communicating with each other. If one value is specified, then that is fixed as the reference core. If two values are specified, only those two cores are tested",
					cxxopts::value<std::vector<std::size_t>>()->default_value(""))
			("b,buffer-size", "Size of the buffer in the shared FIFO in number of messages", cxxopts::value<std::size_t>()->default_value("8"))
			("m,message-size", "Size of the message sent to FIFO in number of cache lines", cxxopts::value<std::size_t>()->default_value("1"))
			("no-save", "If used, gathered data is not stored and does not override any previously gathered data. This is largely for development purposes so you don't kill your SSD")
//			("c,cache-line-size", "Number of bits along which messages are aligned in memory", cxxopts::value<std::size_t>()->default_value("64"))
			("h,help", "Print usage")
			;

	auto result = options.parse(argc, argv);

    if (result.count("help")) {
    	cout << options.help() << endl;
        exit(0);
    }

	cout << "test_mode: " << test_mode << endl;
	cout << "use_avx256: " << use_avx256 << endl;

	const int num_tries = result["numtries"].as<int>();
	const int individual_message = result["individual-message"].as<bool>();
	std::string usr_data_dir = result["data-dir"].as<std::string>();
	const std::vector<std::size_t> specified_cores = result["cores"].as<std::vector<std::size_t>>();
	const std::size_t buffer_size = result["buffer-size"].as<std::size_t>();
	const std::size_t message_size = result["message-size"].as<std::size_t>();
	const bool save = !result["no-save"].as<bool>();

	if (specified_cores.size() > 2) {
		cout << "More than 2 cores specified" << endl;
		exit(0);
	} else if (specified_cores.size() == 2) {
		if (specified_cores[0] == specified_cores[1]) {
			cout << "Same core specified twice" << endl;
			exit(0);
		}
	}
	if (usr_data_dir.empty()) { cout << "PWD as data dir" << endl; }

	std::filesystem::path data_dir =
			usr_data_dir.empty()
			? std::filesystem::current_path() / "data"
			: std::filesystem::path(std::string(usr_data_dir));
	std::filesystem::create_directory(data_dir);

	if (save) { cout << "Data directory = " << data_dir.string() << endl; }

	std::vector<long> dummy(num_tries);

	long t1 = benchmark::get_thread_time_nano(); //Calibration routine
	for (int i = 0; i < num_tries-1; ++i) {
		dummy[i] = benchmark::get_thread_time_nano() - t1;
	}
	long t2 = benchmark::get_thread_time_nano();

	double avg_get_time_cost = (static_cast<double>(t2)-static_cast<double>(t1))/num_tries;

	cout << "Average latency from get_time_nano: " << avg_get_time_cost << endl;

	std::vector<std::string> avg_round_time_nano{};
	avg_round_time_nano.push_back("thread_1_core,thread_2_core,avg_round_time_nano\n"s);

	int max_cores = sysconf(_SC_NPROCESSORS_ONLN);
	cout << "Number of cores detected: " << max_cores << endl;

	for (int core_1 = specified_cores.size() > 0 ? specified_cores[0] : 0; core_1 < max_cores; ++core_1) {
		for (int core_2 = specified_cores.size() > 1 ? specified_cores[1] : 0; core_2 < max_cores; ++core_2) {
			if (core_1 == core_2) {
				continue;
			}
			cout << "Beginning test - core 1: " << core_1 << "  core 2: " << core_2 << endl;
			core_to_core_t<64, test_mode, use_memcpy, use_avx256> ctc{core_1, core_2, num_tries, buffer_size, message_size, !individual_message};

			std::thread thread_1([&]{ctc.thread_1();});
			std::thread thread_2([&]{ctc.thread_2();});
			thread_1.join();
			thread_2.join();

			if (individual_message && save) {
				std::ofstream data;
				std::filesystem::path fileName = data_dir / (std::string("FifoIpcLatency_") + std::to_string(core_1) + std::string("_") + std::to_string(core_2) + std::string(".csv"));
				cout << "Data file: " << fileName << endl;
				data.open(fileName, std::ios_base::trunc);
				data << "attempt,thread_1_round_trip_nano,thread_2_round_trip_nano\n";
				for (int i = 0; i < num_tries; ++i) {
					data << i << "," << ctc.thread_1_round_time_nano[i] - avg_get_time_cost << "," << ctc.thread_2_round_time_nano[i] - avg_get_time_cost << "\n";
				}
				data.close();
				cout << "Saved data -i" << endl;
			} else if (save) {
				std::string temp = std::to_string(core_1) + ","s + std::to_string(core_2) + ","s + std::to_string(ctc.thread_1.avg_latency) + "\n"s;
				avg_round_time_nano.push_back(temp);
			}

			if (!individual_message) {
				cout << "Avg round time latency: " << ctc.thread_1.avg_latency << endl;
			}
			cout << "**********" << endl;


			if (specified_cores.size() == 2) {
				break;
			}
		}
		if (specified_cores.size() >= 1) {
			break;
		}
	}

	if (!individual_message && save) {
		std::ofstream data;
		std::filesystem::path fileName = data_dir / std::string("FifoIpcLatency_avg.csv");
		cout << "Data file: " << fileName << endl;
		data.open(fileName, std::ios_base::trunc);
		for (std::size_t i = 0; i < avg_round_time_nano.size(); ++i) {
			data << avg_round_time_nano[i];
		}
		data.close();
		cout << "Saved data avg" << endl;
	}

	cout << "Done!" << endl;
	return 0;
}
