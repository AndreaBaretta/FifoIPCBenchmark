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
#include <sched.h>
#include <unistd.h>
#include <pthread.h>
#include <errno.h>
#include <time.h>
#include <cstdlib>
#if defined(__x86_64__)
	#include <x86intrin.h>
#endif
#include "fifo.hpp"

namespace benchmark {
	static long get_thread_time_nano() {
		timespec time;
		int res = clock_gettime(CLOCK_THREAD_CPUTIME_ID, &time);
		if (res == 0) {
			return 1000000000*time.tv_sec + time.tv_nsec;
		} else {
			std::cout << "Well shit" << std::endl;
			exit(1);
		}
	}

	static int pin_this_thread_to_core(const int core_id) {
		   int num_cores = sysconf(_SC_NPROCESSORS_ONLN);
		   if (core_id < 0 || core_id >= num_cores) {
			   std::cerr << "pin_this_thread_to_core error - core_id:" << core_id << " num_cores: " << num_cores << std::endl;
			   std::exit(1);
		   }

		   cpu_set_t cpuset;
		   CPU_ZERO(&cpuset);
		   CPU_SET(core_id, &cpuset);

		   auto this_thread = pthread_self();
		   auto result = pthread_setaffinity_np(this_thread, sizeof(cpu_set_t), &cpuset);
		   if (result != 0) {
			   std::cout << "sched_setaffinity error: " << errno << std::endl;
			   std::exit(1);
		   }

		   auto niceness = nice(-20);

		   const auto cpu = sched_getcpu();
		   std::cout << "Running on cpu: " << cpu << " niceness: " << niceness << std::endl;

		   sched_getaffinity(0, sizeof(cpu_set_t), &cpuset);
		   if (CPU_COUNT(&cpuset) != 1 || !CPU_ISSET(core_id, &cpuset)) {
			   std::cout << "cpuset error: count == " << CPU_COUNT(&cpuset) << std::endl;
		   }

//		   sched_param param;
//		   param.sched_priority = 99;
//		   sched_setscheduler(0, SCHED_FIFO, &param);
		   return result;
	}

	static inline volatile unsigned long long rdtsc() {
#if defined(__x86_64__)
		return __rdtsc();
#elif defined(__aarch64__)
		volatile unsigned long long cc;
  		asm volatile ("mrc p15, 0, %0, c9, c13, 0" : "=r" (cc));
  		return cc;
#else
		static_assert(false);
#endif
	} 
}
