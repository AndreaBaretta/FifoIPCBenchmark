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

		std::vector<long> thread_1_round_time_nano;
		std::vector<long> thread_2_round_time_nano;

		using cache_line_type = fifo_t<cache_line_size, use_memcpy, use_avx256>::cache_line_type;
		using aligned_cache_line_type = fifo_t<cache_line_size, use_memcpy, use_avx256>::aligned_cache_line_type;

		class thread_1_t {
			core_to_core_t& core_to_core;
		public:
			long avg_latency;

			thread_1_t(core_to_core_t& core_to_core) : core_to_core(core_to_core), avg_latency(0) {
			};
			thread_1_t(const thread_1_t&) = delete;
			thread_1_t(thread_1_t&&) = delete;

			int operator()() {
				benchmark::pin_this_thread_to_core(core_to_core.core_1);
				std::vector<aligned_cache_line_type> msg_sent{core_to_core.message_size};
				std::vector<aligned_cache_line_type> msg_read{core_to_core.message_size};

				long t1;

				if (!core_to_core.avg) {

					std::cout << "START-T1: Computing avg" << std::endl;

					for (std::size_t i = 0; i < core_to_core.num_tries; ++i) {

						t1 = benchmark::get_thread_time_nano();

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
						core_to_core.thread_1_round_time_nano[i] = benchmark::get_thread_time_nano() - t1;
					}
					std::cout << " END-T1" << std::endl;
				} else {

					std::cout << "START-T1: Computing individual message" << std::endl;
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
					long end = benchmark::get_thread_time_nano();
					avg_latency = (end - start)/static_cast<long>(core_to_core.num_tries);
					const auto cache_to_cache = avg_latency / 6;
					std::cout << " END-T1: end: " << end << "  start: " << start << "  avg round trip: " << avg_latency << " cache_to_cache: " << cache_to_cache << " ns" << std::endl;
					std::cout.flush();
				}

				return 0;
			}
		};

		class thread_2_t {
			core_to_core_t& core_to_core;
		public:
			thread_2_t(core_to_core_t& core_to_core) : core_to_core(core_to_core) {};
			thread_2_t(const thread_1_t&) = delete;
			thread_2_t(thread_1_t&&) = delete;

			int operator()() {
				benchmark::pin_this_thread_to_core(core_to_core.core_2);
				std::vector<aligned_cache_line_type> msg_read{core_to_core.message_size};

				std::cout << "START-T2" << std::endl;
				std::cout.flush();
				long t1;

				if (!core_to_core.avg) {

					for (std::size_t i = 0; i < core_to_core.num_tries; ++i) {
						t1 = benchmark::get_thread_time_nano();
						while (!core_to_core.fifo_1.try_read_message(msg_read)) {}
						while (!core_to_core.fifo_2.try_write_message(msg_read)) {}

						core_to_core.thread_2_round_time_nano[i] = benchmark::get_thread_time_nano() - t1;
					}
				} else {
					for (std::size_t i = 0; i < core_to_core.num_tries; ++i) {
						while (!core_to_core.fifo_1.try_read_message(msg_read)) {}
						while (!core_to_core.fifo_2.try_write_message(msg_read)) {}
					}
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
			avg(avg), thread_1_round_time_nano(num_tries), thread_2_round_time_nano(num_tries), thread_1(*this), thread_2(*this) {}
	};
}
