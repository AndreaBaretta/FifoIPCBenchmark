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
#include "fifo.hpp"
#include "benchmark.hpp"
#include "core_to_core.hpp"
#include "cxxopts.hpp"


#ifdef NDEBUG
constexpr const bool test_mode = false;
#else
constexpr const bool test_mode = true;
#endif

using std::cout;
using std::endl;
using benchmark::fifo_t;
using benchmark::core_to_core_t;


int main(int argc, char** argv) {
	cout << "test_mode: " << test_mode << endl;

	cxxopts::Options options("ThreadToThreadLatency", "Measures the latency of one thread communicating to another");
	options.add_options()
			("n,numtries", "Number of iterations of the test", cxxopts::value<int>()->default_value("1000000"))
			;

	auto result = options.parse(argc, argv);

	cout << "Command line option: " << result["numtries"].as<int>() << endl;

	constexpr const int num_tries = 1000000;
//	const int size_msg = 3;

	cout << "test_mode =" << test_mode << " sizeof(long) =" << sizeof(long) << endl; // prints Hello World!

	long t1 = benchmark::get_thread_time_nano();
	for (int i = 1; i < num_tries-1; ++i) {
		benchmark::get_thread_time_nano();
	}
	long t2 = benchmark::get_thread_time_nano();

	double avg_get_time_cost = (static_cast<double>(t2)-static_cast<double>(t1))/num_tries;

	cout << "Average latency from get_time_nano: " << avg_get_time_cost << endl;


//	for (int core = 1; core < 31; ++core) {
//		cout << "On core: " << core << endl;
//
//		core_to_core_t<64, false> ctc{0, core};
//		std::thread thread_1([&]{ctc.thread_1();});
//		std::thread thread_2([&]{ctc.thread_2();});
//
//		thread_1.join();
//		thread_2.join();
//
//		cout << "Finished threads" << endl;
//
//		std::ofstream data;
//
//		std::string fileName = std::string("/home/andrea/Desktop/InfinityFabricData/InfinityFabricData_0_") + std::to_string(core) + std::string(".csv");
//
//		std::remove(fileName.c_str());
//		data.open(fileName.c_str());
//		data << "attempt,thread_1_round_trip_nano,thread_2_round_trip_nano\n";
//		for (int i = 0; i < num_tries; ++i) {
//			data << i << "," << ctc.thread_1_round_time_nano[i] - avg_get_time_cost << "," << ctc.thread_2_round_time_nano[i] - avg_get_time_cost << "\n";
//		}
//		data.close();
//	}

	for (int core = 1; core < 32; ++core) {
		std::cout << "Beginning core " << core << std::endl;
		core_to_core_t<64, true, test_mode> ctc{0, core, num_tries, 8, 1};

//		cout << "Before creating thread" << endl;
//		long start = latency_measurement_t::get_thread_time_nano();
		std::thread thread_1([&]{ctc.thread_1();});
		std::thread thread_2([&]{ctc.thread_2();});
//		cout << "Before calling join" << endl;
//		cout << "Before join" << endl;
		thread_1.join();
		thread_2.join();

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
