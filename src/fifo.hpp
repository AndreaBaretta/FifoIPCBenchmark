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
#include <cassert>
#include <cstddef>
#include <array>
#include <optional>
#include <vector>
#include <cstring>
#include <x86intrin.h>

#include "memcpy.hpp"

namespace benchmark {

	template <
		std::size_t cache_line_size,
		bool use_memcpy,
		bool use_avx256
	>
	class fifo_t {
	public:
//		static_assert(cache_line_size == 64);
		constexpr static const std::size_t longs_per_cache_line = cache_line_size/sizeof(long);
		using cache_line_type = std::array<long, longs_per_cache_line>;

		struct aligned_cache_line_type {
			static_assert(sizeof(cache_line_type) == cache_line_size);

			alignas(cache_line_size) cache_line_type cache_line;
			const long& operator[](const std::size_t i) const { return cache_line[i]; }
			long& operator[](const std::size_t i) { return cache_line[i]; }
			bool operator==(const aligned_cache_line_type& rhs) const { return cache_line == rhs.cache_line; }
		};

		using buffer_type = aligned_cache_line_type;
		using size_type = std::size_t;
	protected:
		alignas(cache_line_size) volatile size_type write_index = 0;
		alignas(cache_line_size) volatile size_type read_index = 0;
		alignas(cache_line_size) buffer_type *buffer;

		const std::size_t fifo_size;
		const std::size_t message_size;

	public:

		fifo_t(const std::size_t fifo_size, const std::size_t message_size):
			write_index(0), read_index(0), buffer(new buffer_type[fifo_size * message_size]), fifo_size(fifo_size), message_size(message_size) {}

		~fifo_t() {
			delete[] buffer;
		}

		size_type num_messages_to_read() const {
			const size_type result = write_index - read_index;
			assert(result <= fifo_size);
			return result;
		}

		bool can_write() const {
			return num_messages_to_read() < fifo_size;
		}

		bool can_read() const {
			return num_messages_to_read() != 0;
		}

		bool try_write_message(const std::vector<aligned_cache_line_type>& msg) {
			if (!can_write()) {
				return false;
			}

			const std::size_t buffer_index = write_index*message_size;
			if constexpr (use_memcpy) {
				std::memcpy(&buffer[buffer_index%fifo_size], msg.data(), message_size*cache_line_size);
			} else if constexpr (use_avx256) {
				benchmark::copy_data_256<cache_line_size>(&buffer[buffer_index%fifo_size], msg.data(), message_size);
			}
			write_index = write_index + 1;
			return true;
		}

		bool try_read_message(std::vector<aligned_cache_line_type>& msg) {
			if (!can_read()) {
				return false;
			}
			const std::size_t buffer_index = read_index*message_size;
			if constexpr (use_memcpy) {
				std::memcpy(msg.data(), &buffer[buffer_index%fifo_size], message_size*cache_line_size);
			} else if constexpr (use_avx256) {
				benchmark::copy_data_256<cache_line_size>(msg.data(), &buffer[buffer_index%fifo_size], message_size);
			}
			read_index = read_index + 1;
			return true;
		}

	};

}
