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
#include "/usr/src/linux-hwe-5.8-headers-5.8.0-44/include/linux/getcpu.h"

#include <cstdlib>

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

		   //pthread_t current_thread = pthread_self();
		   auto this_thread = pthread_self();
		   auto result = pthread_setaffinity_np(this_thread, sizeof(cpu_set_t), &cpuset);
		   if (result != 0) {
			   std::cout << "sched_setaffinity error: " << errno << std::endl;
			   std::exit(1);
		   }
		   auto niceness = nice(-40);

		   unsigned int cpu;
		   unsigned int node;
		   result = getcpu(&cpu, &node);
//			   std::cout << "Running on cpu: " << cpu << " node: " << node << " niceness: " << niceness << std::endl;

		   sched_getaffinity(0, sizeof(cpu_set_t), &cpuset);
		   if (CPU_COUNT(&cpuset) != 1 || !CPU_ISSET(core_id, &cpuset)) {
			   std::cout << "cpuset error: count == " << CPU_COUNT(&cpuset) << std::endl;
		   }

		   sched_param param;
		   param.sched_priority = 99;
		   sched_setscheduler(0, SCHED_FIFO, &param);
		   return result;
	}

}
