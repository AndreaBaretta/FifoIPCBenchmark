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

using std::cout;
using std::endl;
using benchmark::fifo_t;
using benchmark::latency_measurement_t;
using benchmark::core_to_core_t;

constexpr const int core_writer = 0;
//constexpr const int core_reader = 8;
constexpr const int num_tries = 1000000;

int main() {
	cout << "Hello World!" << endl; // prints Hello World!

	long t1 = latency_measurement_t::get_thread_time_nano();
	for (int i = 1; i < num_tries-1; ++i) {
		latency_measurement_t::get_thread_time_nano();
	}
	long t2 = latency_measurement_t::get_thread_time_nano();

	double avg_get_time_cost = (static_cast<double>(t2)-static_cast<double>(t1))/num_tries;

	cout << "Average latency from get_time_nano: " << avg_get_time_cost << endl;

	for (int core = 1; core < 1; ++core) {
		cout << "On core: " << core << endl;
//		latency_measurement_t lm{core_writer, core, num_tries};
//
//		std::thread writer_thread([&]{lm.writer();});
//		std::thread reader_thread([&]{lm.reader();});
//
//		writer_thread.join();
//		reader_thread.join();
//
//		cout << "Write core: " << core_writer << " Reader core: " << core_reader << endl;
//		cout << "Average write time: " << lm.writer.get_average_time()-avg_get_time_cost << " ns" << endl;
//		cout << "Average read time:  " << lm.reader.get_average_time()-avg_get_time_cost << " ns" << endl;

		core_to_core_t<num_tries, false> ctc{0, core};
		std::thread thread_1([&]{ctc.thread_1();});
		std::thread thread_2([&]{ctc.thread_2();});

		thread_1.join();
		thread_2.join();

		std::ofstream data;

		std::string fileName = std::string("/home/andrea/Desktop/InfinityFabricData/InfinityFabricData_0_") + std::to_string(core) + std::string(".csv");

		std::remove(fileName.c_str());
		data.open(fileName.c_str());
		data << "attempt,thread_1_round_trip_nano,thread_2_round_trip_nano\n";
		for (int i = 0; i < num_tries; ++i) {
			data << i << "," << ctc.thread_1_round_time_nano[i] - avg_get_time_cost << "," << ctc.thread_2_round_time_nano[i] - avg_get_time_cost << "\n";
		}
		data.close();
	}

	for (int core = 1; core < 2; ++core) {
		core_to_core_t<num_tries, true> ctc{0, 8};

//		cout << "Before creating thread" << endl;
		long start = latency_measurement_t::get_thread_time_nano();
		std::thread thread_1([&]{ctc.thread_1();});
		std::thread thread_2([&]{ctc.thread_2();});
//		cout << "Before calling join" << endl;
//		cout << "Before join" << endl;
		thread_1.join();
		thread_2.join();
		cout << "Overall time taken: " << latency_measurement_t::get_thread_time_nano() - start << " ns" << endl;
	}
//	double avgTime = static_cast<double>(latency_measurement_t::get_thread_time_nano()-start)/static_cast<double>(32*1000000);
//	cout << "Average time per round trip: " << avgTime << endl;

	cout << "Done!" << endl;
	return 0;
}
