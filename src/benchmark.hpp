/*
 * benchmark.hpp
 *
 *  Created on: Feb 14, 2021
 *      Author: andrea
 */

#pragma once
#include <sched.h>
#include <unistd.h>
#include <pthread.h>
#include <errno.h>
#include <time.h>

#include "fifo.hpp"

namespace benchmark {
//	template <
//		std::size_t fifo_size,
//		std::size_t cache_line_size
//	>
	class latency_measurement_t {

		fifo_t<char, 8, 64> fifo;

	public:
		const int core_writer;
		const int core_reader;
		const int num_tries;

		class writer_t {
			latency_measurement_t& latency_measurement;
		public:
			long write_time_nano = 0;
			long num_messages = 0;

			writer_t(latency_measurement_t& latency_measurement) : latency_measurement(latency_measurement) {
//				std::cout << "writer_t constructor called: this = " << this << std::endl;
			}
			writer_t(const writer_t&) = delete;
			writer_t(writer_t&&) = delete;

			int operator()() {
				pin_this_thread_to_core(latency_measurement.core_writer);
//				std::cout << "In writer thread, &write_time_nano = " << &write_time_nano << " &num_messages = " << &num_messages << std::endl;
//				std::cout << "In writer thread, num_tries=" << latency_measurement.num_tries << std::endl;
				for (int i = 0; i < latency_measurement.num_tries; ++i) {
					while (!latency_measurement.fifo.can_write()) {}
					long t1 = get_thread_time_nano();
					latency_measurement.fifo.try_write_message('a');
					write_time_nano += get_thread_time_nano() - t1;
					++num_messages;
				}
//				std::cout << "Out of writer thread, num_messages=" << num_messages << ", total time=" << write_time_nano << std::endl;
				return 0;
			}

			double get_average_time() const {
				return static_cast<double>(write_time_nano)/ static_cast<double>(num_messages);
			}

		};

		class reader_t {
			latency_measurement_t& latency_measurement;
		public:
			long read_time_nano = 0;
			long num_messages = 0;

			reader_t(latency_measurement_t& latency_measurement) : latency_measurement(latency_measurement) {
//				std::cout << "reader_t constructor called: this = " << this << std::endl;
			}
			reader_t(const reader_t&) = delete;
			reader_t(reader_t&&) = delete;

			int operator()() {
//				std::cout << "In reader thread, &read_time_nano = " << &read_time_nano << " &num_messages = " << &num_messages << std::endl;
//				std::cout << "In reader thread, num_tries=" << latency_measurement.num_tries << std::endl;
				pin_this_thread_to_core(latency_measurement.core_reader);
				for (int i = 0; i < latency_measurement.num_tries; ++i) {
					while (!latency_measurement.fifo.can_read()) {}
					long t1 = get_thread_time_nano();
					latency_measurement.fifo.try_read_message();
					read_time_nano += get_thread_time_nano() - t1;
					++num_messages;
				}
//				std::cout << "Out of reader thread, num_messages=" << num_messages << ", total time=" << read_time_nano << std::endl;
				return 0;
			}

			double get_average_time() const {
				return (static_cast<double>(read_time_nano))/ (static_cast<double>(num_messages));
			}

		};

		writer_t writer;
		reader_t reader;

		latency_measurement_t(const int core_writer, const int core_reader, const int num_tries) :
			fifo(), core_writer(core_writer), core_reader(core_reader), num_tries(num_tries), writer(*this), reader(*this) {}

	public:
		static long get_thread_time_nano() {
			timespec time;
			int res = clock_gettime(CLOCK_THREAD_CPUTIME_ID, &time);
			if (res == 0) {
				return time.tv_nsec;
			} else {
				std::cout << "Well shit" << std::endl;
				exit(1);
			}
		}

		static int pin_this_thread_to_core(const int core_id) {
			   int num_cores = sysconf(_SC_NPROCESSORS_ONLN);
			   if (core_id < 0 || core_id >= num_cores)
			      return EINVAL;

			   cpu_set_t cpuset;
			   CPU_ZERO(&cpuset);
			   CPU_SET(core_id, &cpuset);

			   pthread_t current_thread = pthread_self();
			   return pthread_setaffinity_np(current_thread, sizeof(cpu_set_t), &cpuset);
		}

	};
}
