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
	class core_to_core_t {

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

		using cache_line_type = typename fifo_t<cache_line_size, use_memcpy, use_avx256>::cache_line_type;
		using aligned_cache_line_type = typename fifo_t<cache_line_size, use_memcpy, use_avx256>::aligned_cache_line_type;

		class thread_1_t {
			core_to_core_t& core_to_core;
		public:
			long avg_latency;
			long tot_time;
			long long tot_cycles;

			thread_1_t(core_to_core_t& core_to_core) : core_to_core(core_to_core), avg_latency(0), tot_time(0), tot_cycles(0) {
			};
			thread_1_t(const thread_1_t&) = delete;
			thread_1_t(thread_1_t&&) = delete;

			int operator()() {
				benchmark::init_rdtsc();
				benchmark::pin_this_thread_to_core(core_to_core.core_1);
				std::vector<aligned_cache_line_type> msg_sent{core_to_core.message_size};
				std::vector<aligned_cache_line_type> msg_read{core_to_core.message_size};

				long long t1;
				long long t2;

				if (!core_to_core.avg) {

					std::cout << "START-T1: Computing individual message" << std::endl;

					long start_time = benchmark::get_thread_time_nano();
					long long start_cycles = benchmark::rdtsc();

					for (std::size_t i = 0; i < core_to_core.num_tries; ++i) {
						msg_sent[0][0] = i;

						t1 = benchmark::rdtsc();
						while (!core_to_core.fifo_1.try_write_message(msg_sent)) {}
						while (!core_to_core.fifo_2.try_read_message(msg_read)) {}
						t2 = benchmark::rdtsc(); 

						core_to_core.thread_1_round_time_cycles[i] = t2 - t1 - 1;

						if constexpr (test_mode) {
							if (msg_read != msg_sent) {
								std::cout << "Failed at i = " << i << std::endl;
								std::cout << "Expecting: ";
								for (const auto& aligned_cache_line : msg_sent) {
									for (const long v : aligned_cache_line.cache_line) std::cout << v << ", ";
								}
								std::cout << std::endl;
								std::cout << " Received: ";
								for (const auto& aligned_cache_line : msg_read) {
									for (const long v : aligned_cache_line.cache_line) std::cout << v << ", ";
								}
								std::cout << std::endl;
								throw std::string("Bah humbug");
							}
						} 

					}
					tot_cycles = benchmark::rdtsc() - start_cycles - core_to_core.num_tries;
					tot_time = benchmark::get_thread_time_nano() - start_time;
					std::cout << " END-T1: Total cycles: " << tot_cycles << std::endl;
				} else {

					std::cout << "START-T1: Computing average" << std::endl;
					std::cout.flush();

					long start = benchmark::get_thread_time_nano();

					for (std::size_t i = 0; i < core_to_core.num_tries; ++i) {

						msg_sent[0][0] = i;

						while (!core_to_core.fifo_1.try_write_message(msg_sent)) {}
						while (!core_to_core.fifo_2.try_read_message(msg_read)) {}

						if constexpr (test_mode) {
							if (msg_read != msg_sent) {
								std::cout << "Failed at i = " << i << std::endl;
								std::cout << "Expecting: ";
								for (const auto& aligned_cache_line : msg_sent) {
									for (const long v : aligned_cache_line.cache_line) std::cout << v << ", ";
								}
								std::cout << std::endl;
								std::cout << " Received: ";
								for (const auto& aligned_cache_line : msg_read) {
									for (const long v : aligned_cache_line.cache_line) std::cout << v << ", ";
								}
								std::cout << std::endl;
								throw std::string("Bah humbug");
							}
						}
					}
					tot_time = benchmark::get_thread_time_nano() - start;
					avg_latency = tot_time/static_cast<long>(core_to_core.num_tries);
					std::cout << " END-T1: avg round trip: " << avg_latency << " ns" << std::endl;
					std::cout.flush();
				}

				return 0;
			}
		};

		class thread_2_t {
			core_to_core_t& core_to_core;
		public:
			long tot_time;
			long long tot_cycles;

			thread_2_t(core_to_core_t& core_to_core) : core_to_core(core_to_core), tot_time(0), tot_cycles(0) {};
			thread_2_t(const thread_1_t&) = delete;
			thread_2_t(thread_1_t&&) = delete;

			int operator()() {
				benchmark::init_rdtsc();
				benchmark::pin_this_thread_to_core(core_to_core.core_2);
				std::vector<aligned_cache_line_type> msg_read{core_to_core.message_size};

				std::cout << "START-T2" << std::endl;
				std::cout.flush();
				long long t1;
				long long t2;

				if (!core_to_core.avg) {
					long start_time = benchmark::get_thread_time_nano();
					long long start_cycles = benchmark::rdtsc();
					for (std::size_t i = 0; i < core_to_core.num_tries; ++i) {
						t1 = benchmark::rdtsc();
						while (!core_to_core.fifo_1.try_read_message(msg_read)) {}
						while (!core_to_core.fifo_2.try_write_message(msg_read)) {}
						t2 = benchmark::rdtsc();
						core_to_core.thread_2_round_time_cycles[i] = t2 - t1 - 1;
					}
					tot_cycles = benchmark::rdtsc() - start_cycles - core_to_core.num_tries;
					tot_time = benchmark::get_thread_time_nano() - start_time;
					std::cout << "Setting total time: " << tot_time << std::endl;
				} else {
					long start = benchmark::get_thread_time_nano();
					for (std::size_t i = 0; i < core_to_core.num_tries; ++i) {
						while (!core_to_core.fifo_1.try_read_message(msg_read)) {}
						while (!core_to_core.fifo_2.try_write_message(msg_read)) {}
					}
					tot_time = benchmark::get_thread_time_nano() - start;
				}

				std::cout << " END-T2" << std::endl;
				std::cout.flush();
				return 0;
			}
		};

		thread_1_t thread_1;
		thread_2_t thread_2;

		core_to_core_t(const int core_1, const int core_2, const int num_tries, const std::size_t fifo_size, const std::size_t message_size, const bool avg) :
			fifo_1(fifo_size, message_size), fifo_2(fifo_size, message_size), core_1(core_1), core_2(core_2), num_tries(num_tries), fifo_size(fifo_size), message_size(message_size),
			avg(avg), thread_1_round_time_cycles(num_tries), thread_2_round_time_cycles(num_tries), thread_1(*this), thread_2(*this) {}
	};
}
