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
along with FifoIPCBenchmark.  If not, see <https://www.gnu.org/licenses/>.

*/

#pragma once

#include <cassert>
#include <cstddef>
#include <x86intrin.h>

namespace benchmark {

	template <
		std::size_t cache_line_size
	>
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
