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

		std::array<int, num_tries> thread_1_round_time_nano;
		std::array<int, num_tries> thread_2_round_time_nano;

		class thread_1_t {
			core_to_core_t& core_to_core;
		public:
			thread_1_t(core_to_core_t& core_to_core) : core_to_core(core_to_core) {
			};
			thread_1_t(const thread_1_t&) = delete;
			thread_1_t(thread_1_t&&) = delete;

			int operator()() {
//				std::cout << "START(1)" << std::endl;
				latency_measurement_t::pin_this_thread_to_core(core_to_core.core_1);
//				std::cout << "PINNED(1)" << std::endl;
				long t1;
				for (std::size_t i = 0; i < num_tries; ++i) {
//					const bool do_log = (i%10000==0);
//					if (do_log) std::cout << '.';
					if constexpr (!avg) {
						t1 = latency_measurement_t::get_thread_time_nano();
					}
					while (!core_to_core.fifo_1.try_write_message('a')) {
					}
					while (!core_to_core.fifo_2.try_read_message().has_value()) {
					}
					if constexpr (!avg) {
						core_to_core.thread_1_round_time_nano[i] = latency_measurement_t::get_thread_time_nano() - t1;
					}
				}
				std::stringstream tmp;
				tmp << " DONE(1) " << std::endl;
				std::cout << tmp.str();
				return 0;
			}
		};

		class thread_2_t {
			core_to_core_t& core_to_core;
		public:
			thread_2_t(core_to_core_t& core_to_core) : core_to_core(core_to_core) {
			};
			thread_2_t(const thread_1_t&) = delete;
			thread_2_t(thread_1_t&&) = delete;

			int operator()() {
//				std::cout << "START(2)" << std::endl;
				latency_measurement_t::pin_this_thread_to_core(core_to_core.core_2);
//				std::cout << "PINNED(2)" << std::endl;
				long t1;
				long start = latency_measurement_t::get_thread_time_nano();
				for (std::size_t i = 0; i < num_tries; ++i) {
//					const bool do_log = (i%10000==0);
//					if (do_log) std::cout << '-';
					if constexpr (!avg) {
						t1 = latency_measurement_t::get_thread_time_nano();
					}
					while (!core_to_core.fifo_1.try_read_message().has_value()) {
					}
					while (!core_to_core.fifo_2.try_write_message('a')) {
					}
					if constexpr (!avg) {
						core_to_core.thread_2_round_time_nano[i] = latency_measurement_t::get_thread_time_nano() - t1;
					}
				}
//				std::cout << "Out of thread_2" << std::endl;
				long end = latency_measurement_t::get_thread_time_nano();
				std::stringstream tmp;
				tmp << " DONE(2) time=" << (end - start) << std::endl;
				std::cout << tmp.str();
				return 0;
			}
		};

		thread_1_t thread_1;
		thread_2_t thread_2;

		core_to_core_t(const int core_1, const int core_2) :
			fifo_1(), fifo_2(), core_1(core_1), core_2(core_2), thread_1_round_time_nano(), thread_2_round_time_nano(), thread_1(*this), thread_2(*this) {}
	};
}
