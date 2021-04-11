/*
 * memcpy.hpp
 *
 *  Created on: Mar 20, 2021
 *      Author: andrea
 */
#pragma once

#include <cassert>
#include <cstddef>
#include <x86intrin.h>

namespace benchmark {

	template <
		std::size_t cache_line_size
	>
		//	void copy_data_256(void* dst, const void* src, size_t size) {
		//		assert(size % 32 == 0);
		//		while(size) {
		//			_mm256_store_si256 ((__m256i*)dst, _mm256_load_si256((__m256i const*)src));
		//			src += 32;
		//			dst += 32;
		//			size -= 32;
		//		}
		//	}
	void copy_data_256(void* dst, const void* src, const std::size_t message_size) {
		static_assert(cache_line_size % 32 == 0);
		std::size_t n_iter = (cache_line_size/32)*message_size;
		for (std::size_t i = 0; i < n_iter; ++i) {
			_mm256_store_si256 ((__m256i*)dst, _mm256_load_si256((__m256i const*)src));
			src = reinterpret_cast<const char*>(src) + 32;
			dst = reinterpret_cast<char*>(dst) + 32;
		}
	}

}
