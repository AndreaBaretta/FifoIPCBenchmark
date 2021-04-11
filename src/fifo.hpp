/*
 * fifo.hpp
 *
 *  Created on: Feb 13, 2021
 *      Author: andrea
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

#define USE_VOLATILE

#ifdef USE_VOLATILE
#  define VOLATILE volatile
#else
#  define VOLATILE
#endif

#define STRINGIFY2(x) #x
#define STRINGIFY(x) STRINGIFY2(x)

namespace benchmark {
//#ifdef NDEBUG
//	void copy_data_256(void* dst, const void* src, size_t size) {
//		assert(size % 32 == 0);
//		while(size) {
//			_mm256_store_si256 ((__m256i*)dst, _mm256_load_si256((__m256i const*)src));
//			src += 32;
//			dst += 32;
//			size -= 32;
//		}
//	}
//#endif

	template <
		std::size_t cache_line_size,
		bool use_memcpy,
		bool use_avx256
	>
	class fifo_t {
	public:
		static_assert(cache_line_size == 64);
		constexpr static const std::size_t longs_per_cache_line = cache_line_size/sizeof(long);
		using cache_line_type = std::array<long, longs_per_cache_line>;

		struct aligned_cache_line_type {
			static_assert(sizeof(cache_line_type) == cache_line_size);

			alignas(cache_line_size) cache_line_type cache_line;
			const long& operator[](const std::size_t i) const { return cache_line[i]; }
			long& operator[](const std::size_t i) { return cache_line[i]; }
			bool operator==(const aligned_cache_line_type& rhs) const { return cache_line == rhs.cache_line; }

			//aligned_message_type& operator=(VOLATILE message_type&& msg) VOLATILE { this->msg = std::forward<message_type>(msg); return *this; }

			//operator cache_line_type() const VOLATILE { return cache_line; }
		};

		using buffer_type = aligned_cache_line_type;
		using size_type = std::size_t;
	protected:
		alignas(cache_line_size) VOLATILE size_type write_index = 0;
		alignas(cache_line_size) VOLATILE size_type read_index = 0;

		/*
		 * int * foo;
		 * foo = new int [5];
		 */
		alignas(cache_line_size) buffer_type *buffer;

		const std::size_t fifo_size;
		const std::size_t message_size;

	public:

		fifo_t(const std::size_t fifo_size, const std::size_t message_size):
			write_index(0), read_index(0), buffer(new buffer_type[fifo_size * message_size]), fifo_size(fifo_size), message_size(message_size) {
			std::cout << "Volatile: " << STRINGIFY(VOLATILE) << std::endl;
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
//			assert(msg.size() == message_size);
//			std::cout << "In try_write_message, can_write: " << can_write() << ", write index: " << write_index << ", read index: " << read_index << std::endl;

			if (!can_write()) {
//				std::cout << "I give up" << std::endl;
				return false;
			}

			const std::size_t buffer_index = write_index*message_size;
			if constexpr (use_memcpy) {
				std::memcpy(&buffer[buffer_index%fifo_size], msg.data(), message_size*cache_line_size);
			} else if constexpr (use_avx256) {
				benchmark::copy_data_256<cache_line_size>(&buffer[buffer_index%fifo_size], msg.data(), message_size);
			} else {
				static_assert(use_memcpy || use_avx256);
//				for (std::size_t i = 0; i < message_size; ++i) {
//					buffer[(buffer_index + i)%fifo_size] = msg[i];
//				}
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
			} else {
				for (std::size_t i = 0; i < message_size; ++i) {
					msg[i] = buffer[(buffer_index + i)%fifo_size];
				}
			}
			read_index = read_index + 1;
			return true;
		}

	};

}
