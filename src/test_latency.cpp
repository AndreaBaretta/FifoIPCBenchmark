//============================================================================
// Name        : test_latency.cpp
// Author      : 
// Version     :
// Copyright   :
// Description : Measure the latency of Infinity Fabric
//============================================================================

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
			("no-save", "If used, gathered data is not stored and does not override any previously gathered data. This is largely for development purposes so I don't kill my SSD")
//			("c,cache-line-size", "Number of bits along which the messages are aligned in memory", cxxopts::value<std::size_t>()->default_value("64"))
			("h,help", "Print usage")
			;

	auto result = options.parse(argc, argv);

    if (result.count("help")) {
    	std::cout << options.help() << std::endl;
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

	cout << "Buffer size: " << buffer_size << endl;
	cout << "Message size: " << message_size << endl;

	cout << "specified_cores: ";
	for (std::size_t i = 0; i < specified_cores.size(); ++i) { cout << specified_cores[i] << ", "; }
	cout << endl;

	if (specified_cores.size() > 2) {
		cout << "More than 2 cores specified" << endl;
		exit(0);
	} else if (specified_cores.size() == 2) {
		if (specified_cores[0] == specified_cores[1]) {
			cout << "Same core specified twice" << endl;
			exit(0);
		}
	}

//	cout << "Command line option: Tries=" << num_tries << "  Individual message=" << individual_message << "  User data directory=" << usr_data_dir << endl;
	if (usr_data_dir.empty()) { cout << "PWD as data dir" << endl; }

//	const int size_msg = 3;

	cout << "test_mode =" << test_mode << " sizeof(long) =" << sizeof(long) << endl; // prints Hello World!

	std::vector<long> dummy(num_tries);

	long t1 = benchmark::get_thread_time_nano();
	for (int i = 0; i < num_tries-1; ++i) {
		dummy[i] = benchmark::get_thread_time_nano() - t1;
	}
	long t2 = benchmark::get_thread_time_nano();

	double avg_get_time_cost = (static_cast<double>(t2)-static_cast<double>(t1))/num_tries;

	cout << "Average latency from get_time_nano: " << avg_get_time_cost << "dummy size: " << dummy.size() << endl;

	std::filesystem::path data_dir =
			usr_data_dir.empty()
			? std::filesystem::current_path() / "data"
			: std::filesystem::path(std::string(usr_data_dir));
	std::filesystem::create_directory(data_dir);
	cout << "Data directory=" << data_dir.string() << endl;

	std::vector<std::string> avg_round_time_nano{};
	avg_round_time_nano.push_back("thread_1_core,thread_2_core,avg_round_time_nano\n"s);

	for (int core_1 = specified_cores.size() > 0 ? specified_cores[0] : 0; core_1 < 32; ++core_1) {
		for (int core_2 = specified_cores.size() > 1 ? specified_cores[1] : 0; core_2 < 32; ++core_2) {
			if (core_1 == core_2) {
				continue;
			}
			cout << "Core 1: " << core_1 << "  core 2: " << core_2 << endl;
	//		std::cout << "Beginning core " << core << std::endl;
			core_to_core_t<64, test_mode, use_memcpy, use_avx256> ctc{core_1, core_2, num_tries, buffer_size, message_size, !individual_message};

	//		cout << "Before creating thread" << endl;
	//		long start = latency_measurement_t::get_thread_time_nano();
			std::thread thread_1([&]{ctc.thread_1();});
			std::thread thread_2([&]{ctc.thread_2();});
	//		cout << "Before calling join" << endl;
	//		cout << "Before join" << endl;
			thread_1.join();
			thread_2.join();

			if (individual_message && save) {
				std::ofstream data;
				std::filesystem::path fileName = data_dir / (std::string("FifoIpcLatency_") + std::to_string(core_1) + std::string("_") + std::to_string(core_2) + std::string(".csv"));
				cout << "fileName: " << fileName << endl;

	//			std::remove(fileName.c_str());
				data.open(fileName, std::ios_base::trunc);
				data << "attempt,thread_1_round_trip_nano,thread_2_round_trip_nano\n";
				for (int i = 0; i < num_tries; ++i) {
					data << i << "," << ctc.thread_1_round_time_nano[i] - avg_get_time_cost << "," << ctc.thread_2_round_time_nano[i] - avg_get_time_cost << "\n";
				}
				data.close();
				cout << "Saved data -i" << endl;
			} else if (save) {
	//			std::string temp = std::string("");// + 0 + ","s + core + ","s + ctc.thread_1.avg_latency;
				std::string temp = std::to_string(core_1) + ","s + std::to_string(core_2) + ","s + std::to_string(ctc.thread_1.avg_latency) + "\n"s;
	//			temp += std::to_string(core&) + ","s;
				cout << "temp: " << temp << endl;
				avg_round_time_nano.push_back(temp);
			}

			cout << "Avg round time latency: " << ctc.thread_1.avg_latency << endl;
	//		cout << "Overall time taken: " << latency_measurement_t::get_thread_time_nano() - start << " ns" << endl;
	//		cout << "Finished avg!" << endl;
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
		cout << "fileName: " << fileName << endl;
		data.open(fileName, std::ios_base::trunc);
		for (std::size_t i = 0; i < avg_round_time_nano.size(); ++i) {
			data << avg_round_time_nano[i];
		}
		data.close();
		cout << "Saved data avg" << endl;
	}
//	double avgTime = static_cast<double>(latency_measurement_t::get_thread_time_nano()-start)/static_cast<double>(32*1000000);
//	cout << "Average time per round trip: " << avgTime << endl;

	cout << "Done!" << endl;
	return 0;
}

//int main(int, char**) {
//	core_to_core_t<1000000, false> ctc{0, 1};
//	std::cout << "Success! size = " << sizeof(ctc) << " sizeof(thread)=" << sizeof(decltype([&]{ctc.thread_1();})) << std::endl;
//	unmain(0, nullptr);
//}
