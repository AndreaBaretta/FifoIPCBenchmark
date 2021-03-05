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

namespace benchmark {

	template <
		std::size_t cache_line_size
	>
	class fifo_t {
		using cache_line_type = std::array<long, cache_line_size/sizeof(long)>;

		struct aligned_cache_line_type {
			static_assert(sizeof(cache_line_type) == cache_line_size);

			alignas(cache_line_size) cache_line_type cache_line;

			volatile aligned_cache_line_type& operator=(const volatile cache_line_type & msg) volatile { this->msg = msg; return *this; }
			//aligned_message_type& operator=(volatile message_type&& msg) volatile { this->msg = std::forward<message_type>(msg); return *this; }

			operator cache_line_type() const volatile { return cache_line; }
		};

		using buffer_type = std::vector<volatile aligned_cache_line_type>;
		using size_type = std::size_t;

		alignas(cache_line_size) volatile size_type write_index = 0;
		alignas(cache_line_size) volatile size_type read_index = 0;
		alignas(cache_line_size) buffer_type buffer{};

		const std::size_t fifo_size;
		const std::size_t message_size;

	public:

		fifo_t(const std::size_t fifo_size, const std::size_t message_size):
			write_index(0), read_index(0), buffer(fifo_size*message_size), fifo_size(fifo_size), message_size(message_size) {}

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

		bool try_write_message(const std::vector<cache_line_type>& msg) {
			assert(msg.size() == message_size);
			if (!can_write()) {
				return false;
			}
			const std::size_t buffer_index = write_index*message_size;
			for (int i = 0; i < message_size; ++i) {
				buffer[(buffer_index + i)%fifo_size] = msg[i];
			}
			++write_index;
			return true;
		}

		std::optional<std::vector<cache_line_type>> try_read_message() {
			if (!can_read()) {
				return {};
			}
			std::vector<cache_line_type> msg{message_size};
			const std::size_t buffer_index = read_index*message_size;
			for (int i = 0; i < message_size; ++i) {
				msg[i] = buffer[(buffer_index + i)%fifo_size];
			}
			++read_index;
			return msg;
		}

	};

}
