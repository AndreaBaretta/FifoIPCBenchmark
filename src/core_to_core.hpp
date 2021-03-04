/*
 * core_to_core.hpp
 *
 *  Created on: Feb 17, 2021
 *      Author: andrea
 */

#pragma once

#include "fifo.hpp"
#include "benchmark.hpp"
#include <sstream>
#include <vector>

namespace benchmark {

	using benchmark::fifo_t;
	using benchmark::latency_measurement_t;

	template <
		std::size_t num_tries,
		bool avg
	>
	class core_to_core_t {

		fifo_t<char, 8, 64> fifo_1;
		fifo_t<char, 8, 64> fifo_2;

	public:
		const int core_1;
		const int core_2;

		std::vector<int> thread_1_round_time_nano;
		std::vector<int> thread_2_round_time_nano;

		class thread_1_t {
			core_to_core_t& core_to_core;
		public:
			thread_1_t(core_to_core_t& core_to_core) : core_to_core(core_to_core) {
			};
			thread_1_t(const thread_1_t&) = delete;
			thread_1_t(thread_1_t&&) = delete;

			int operator()() {
				latency_measurement_t::pin_this_thread_to_core(core_to_core.core_1);

				for (std::size_t i = 0; i < num_tries; ++i) {
					while (!core_to_core.fifo_1.try_write_message('a')) {}
					while (!core_to_core.fifo_2.try_read_message().has_value()) {}
				}

				long t1;
				long start = latency_measurement_t::get_thread_time_nano();
				for (std::size_t i = 0; i < num_tries; ++i) {
					if constexpr (!avg) {
						t1 = latency_measurement_t::get_thread_time_nano();
					}
					while (!core_to_core.fifo_1.try_write_message('a')) {}
					while (!core_to_core.fifo_2.try_read_message().has_value()) {}

					if constexpr (!avg) {
						core_to_core.thread_1_round_time_nano[i] = latency_measurement_t::get_thread_time_nano() - t1;
					}
				}
				long end = latency_measurement_t::get_thread_time_nano();
				std::cout << "Thread 1 core: " << core_to_core.core_2 << " end: " << end << "  start: " << start << "  avg: " << (end - start)/static_cast<long>(num_tries) << " ns" << std::endl;
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
				latency_measurement_t::pin_this_thread_to_core(core_to_core.core_2);
				for (std::size_t i = 0; i < (avg ? 2*num_tries : num_tries); ++i) {
					while (!core_to_core.fifo_1.try_read_message().has_value()) {}
					while (!core_to_core.fifo_2.try_write_message('a')) {}
				}

				if constexpr (!avg) {
					long t1;
					for (std::size_t i = 0; i < num_tries; ++i) {
						t1 = latency_measurement_t::get_thread_time_nano();
						while (!core_to_core.fifo_1.try_read_message().has_value()) {}
						while (!core_to_core.fifo_2.try_write_message('a')) {}
						core_to_core.thread_2_round_time_nano[i] = latency_measurement_t::get_thread_time_nano() - t1;
					}
				}
				return 0;
			}
		};

		thread_1_t thread_1;
		thread_2_t thread_2;

		core_to_core_t(const int core_1, const int core_2) :
			core_1(core_1), core_2(core_2), thread_1_round_time_nano(num_tries), thread_2_round_time_nano(num_tries),
			fifo_1(), fifo_2(), thread_1(*this), thread_2(*this) {}
	};
}
