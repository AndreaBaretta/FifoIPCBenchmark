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


int main(int argc, char** argv) {
	cxxopts::Options options("ThreadToThreadLatency", "Measures the latency of one thread communicating to another over a shared memory FIFO");
	options.add_options()
			("n,numtries", "Number of iterations of the test", cxxopts::value<int>()->default_value("1000000"))
			("i,individual-message", "Save latency data on individual messages")
			("d,data-dir", "Directory for data on individual messages. Useless without -i. If not specified, uses present working directory", cxxopts::value<std::string>()->default_value(std::string("")))
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

	cout << "Command line option: Tries=" << num_tries << "  Individual message=" << individual_message << "  User data directory=" << usr_data_dir << endl;
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

	std::filesystem::path data_dir;
	if (individual_message) {
		data_dir =
			usr_data_dir.empty()
			? std::filesystem::current_path() / "data"
			: std::filesystem::path(std::string(usr_data_dir));
		std::filesystem::create_directory(data_dir);
		cout << "Data directory=" << data_dir.string() << endl;
	}


	for (int core = 1; core < 32; ++core) {
//		std::cout << "Beginning core " << core << std::endl;
		core_to_core_t<64, test_mode, use_memcpy, use_avx256> ctc{0, core, num_tries, 8, 1, !individual_message};

//		cout << "Before creating thread" << endl;
//		long start = latency_measurement_t::get_thread_time_nano();
		std::thread thread_1([&]{ctc.thread_1();});
		std::thread thread_2([&]{ctc.thread_2();});
//		cout << "Before calling join" << endl;
//		cout << "Before join" << endl;
		thread_1.join();
		thread_2.join();

		if (individual_message) {
			std::ofstream data;

			std::filesystem::path fileName = data_dir / (std::string("FifoIpcLatency_0_") + std::to_string(core) + std::string(".csv"));
			cout << "fileName: " << fileName << endl;

//			std::remove(fileName.c_str());
			data.open(fileName, std::ios_base::trunc);
			data << "attempt,thread_1_round_trip_nano,thread_2_round_trip_nano\n";
			for (int i = 0; i < num_tries; ++i) {
				data << i << "," << ctc.thread_1_round_time_nano[i] - avg_get_time_cost << "," << ctc.thread_2_round_time_nano[i] - avg_get_time_cost << "\n";
			}
			data.close();
		}

//		cout << "Overall time taken: " << latency_measurement_t::get_thread_time_nano() - start << " ns" << endl;
//		cout << "Finished avg!" << endl;
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
