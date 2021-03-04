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

namespace benchmark {

	template <
		typename message_type,
		std::size_t fifo_size,
		std::size_t cache_line_size
	>
	class fifo_t {
		struct aligned_message_type {
			alignas(cache_line_size) message_type msg;

			volatile aligned_message_type& operator=(const volatile message_type& msg) volatile { this->msg = msg; return *this; }
			//aligned_message_type& operator=(volatile message_type&& msg) volatile { this->msg = std::forward<message_type>(msg); return *this; }

			operator message_type() const volatile { return msg; }
		};

		using buffer_type = std::array<volatile aligned_message_type, fifo_size>;
		using size_type = std::size_t;

		alignas(cache_line_size) volatile size_type write_index = 0;
		alignas(cache_line_size) volatile size_type read_index = 0;

		alignas(cache_line_size) buffer_type buffer;

	public:

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

		bool try_write_message(const message_type& msg) {
			if (!can_write()) {
				return false;
			}
			buffer[write_index%fifo_size] = msg;
			++write_index;
			return true;
		}

		std::optional<message_type> try_read_message() {
			if (!can_read()) {
				return {};
			}
			return buffer[(read_index++)%fifo_size];
		}

	};

}
