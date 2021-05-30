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

#pragma once

#include "fifo.hpp"
#include "benchmark.hpp"
#include <sstream>
#include <vector>
#include <optional>

namespace benchmark {

	template <
		std::size_t cache_line_size,
		bool test_mode,
		bool use_memcpy,
		bool use_avx256
	>
	class throughput_threads_t {

		fifo_t<cache_line_size, use_memcpy, use_avx256> fifo_1;
		fifo_t<cache_line_size, use_memcpy, use_avx256> fifo_2;

	public:
		const std::size_t core_1;
		const std::size_t core_2;
		const std::size_t num_tries;

		const std::size_t fifo_size;
		const std::size_t message_size;

		const bool avg;

		std::vector<long long> thread_1_round_time_cycles;
		std::vector<long long> thread_2_round_time_cycles;

		std::atomic<bool>& start;

		using cache_line_type = typename fifo_t<cache_line_size, use_memcpy, use_avx256>::cache_line_type;
		using aligned_cache_line_type = typename fifo_t<cache_line_size, use_memcpy, use_avx256>::aligned_cache_line_type;

		class thread_1_t {
			throughput_threads_t& throughput_threads;
			std::thread thread_1;
		public:
			long avg_latency;
			long tot_time;
			long long tot_cycles;

			thread_1_t(throughput_threads_t& throughput_threads) : throughput_threads(throughput_threads), avg_latency(0), tot_time(0), tot_cycles(0) {
			};
			thread_1_t(const thread_1_t&) = delete;
			thread_1_t(thread_1_t&&) = delete;

			int operator()() {
				benchmark::init_rdtsc();
				benchmark::pin_this_thread_to_core(throughput_threads.core_1);

				while (!throughput_threads.start) {}

				std::vector<aligned_cache_line_type> msg_sent{throughput_threads.message_size};

				long long t1;
				long long t2;

				if (!throughput_threads.avg) {

					std::cout << "START-T1: Computing individual message" << std::endl;

					long start_time = benchmark::get_thread_time_nano();
					long long start_cycles = benchmark::rdtsc();

					for (std::size_t i = 0; i < throughput_threads.num_tries; ++i) {
						msg_sent[0][0] = i;

						t1 = benchmark::rdtsc();
						while (!throughput_threads.fifo_1.try_write_message(msg_sent)) {}
						t2 = benchmark::rdtsc(); 

						throughput_threads.thread_1_round_time_cycles[i] = t2 - t1 - 1;
					}
					tot_cycles = benchmark::rdtsc() - start_cycles - throughput_threads.num_tries;
					tot_time = benchmark::get_thread_time_nano() - start_time;
					std::cout << " END-T1: Total cycles: " << tot_cycles << std::endl;
				} else {

					std::cout << "START-T1: Computing average" << std::endl;
					std::cout.flush();

					long start = benchmark::get_thread_time_nano();

					for (std::size_t i = 0; i < throughput_threads.num_tries; ++i) {
						msg_sent[0][0] = i;

						while (!throughput_threads.fifo_1.try_write_message(msg_sent)) {}
					}
					tot_time = benchmark::get_thread_time_nano() - start;
					avg_latency = tot_time/static_cast<long>(throughput_threads.num_tries);
					std::cout << " END-T1: avg single message latency: " << avg_latency << " ns" << std::endl;
					std::cout.flush();
				}

				return 0;
			}
		};

		class thread_2_t {
			throughput_threads_t& throughput_threads;
		public:
			long tot_time;
			long long tot_cycles;

			thread_2_t(throughput_threads_t& throughput_threads) : throughput_threads(throughput_threads), tot_time(0), tot_cycles(0) {};
			thread_2_t(const thread_1_t&) = delete;
			thread_2_t(thread_1_t&&) = delete;

			int operator()() {
				benchmark::init_rdtsc();
				benchmark::pin_this_thread_to_core(throughput_threads.core_2);

				while (!throughput_threads.start) {}

				std::vector<aligned_cache_line_type> msg_read{throughput_threads.message_size};

				// std::cout << "START-T2: Core: " << throughput_threads.core_2 << std::endl;
				// std::cout.flush();
				long long t1;
				long long t2;

				if (!throughput_threads.avg) {
					long start_time = benchmark::get_thread_time_nano();
					long long start_cycles = benchmark::rdtsc();
					for (std::size_t i = 0; i < throughput_threads.num_tries; ++i) {
						t1 = benchmark::rdtsc();
						while (!throughput_threads.fifo_1.try_read_message(msg_read)) {}
						t2 = benchmark::rdtsc();
						throughput_threads.thread_2_round_time_cycles[i] = t2 - t1 - 1;
					}
					tot_cycles = benchmark::rdtsc() - start_cycles - throughput_threads.num_tries;
					tot_time = benchmark::get_thread_time_nano() - start_time;
				} else {
					long start = benchmark::get_thread_time_nano();
					for (std::size_t i = 0; i < throughput_threads.num_tries; ++i) {
						while (!throughput_threads.fifo_1.try_read_message(msg_read)) {}
					}
					tot_time = benchmark::get_thread_time_nano() - start;
				}

				// std::cout << " END-T2" << std::endl;
				// std::cout.flush();
				return 0;
			}
		};

		thread_1_t thread_1;
		thread_2_t thread_2;

		throughput_threads_t(const int core_1, const int core_2, const int num_tries, const std::size_t fifo_size, const std::size_t message_size, const bool avg, std::atomic<bool>& start) :
			fifo_1(fifo_size, message_size), fifo_2(fifo_size, message_size), core_1(core_1), core_2(core_2), num_tries(num_tries), fifo_size(fifo_size), message_size(message_size),
			avg(avg), thread_1_round_time_cycles(num_tries), thread_2_round_time_cycles(num_tries), start(start), thread_1(*this), thread_2(*this) {}
	};
}
