#ifndef RING_BUFFER_H_
#define RING_BUFFER_H_
#include <mutex>
#include <array>
#include <algorithm>
#include <stdexcept>
#include <cassert>
#include "common/NoCopyable.h"
namespace Render::Common {

	class RingBufferAllocator : public NonCopyable {
	public:
		struct Range { size_t offset; size_t size; };

		struct Reservation {
			std::array<Range, 1> parts;
			int count = 0;
			size_t total_advance = 0;
		};

		RingBufferAllocator(size_t capacity, size_t alignment, bool allow_wrap_to_zero = true, bool thread_safe = false)
			: m_capacity(capacity),
			m_alignment(alignment == 0 ? 1 : alignment),
			m_allow_wrap(allow_wrap_to_zero),
			m_thread_safe(thread_safe),
			m_head(0), m_tail(0), m_used(0)
		{
			if (m_capacity == 0) throw std::invalid_argument("Capacity must be > 0");
			if ((m_alignment & (m_alignment - 1)) != 0) throw std::invalid_argument("Alignment must be power of two");
			if (m_capacity % m_alignment != 0) throw std::invalid_argument("Capacity must be multiple of alignment");
		}

		size_t capacity() const noexcept { return m_capacity; }
		size_t alignment() const noexcept { return m_alignment; }
		size_t used() const noexcept { return m_used; }
		size_t head_offset() const noexcept { return m_head; }
		size_t tail_offset() const noexcept { return m_tail; }

		size_t free_space() const noexcept { return m_capacity - m_used; }

		static inline size_t round_up(size_t v, size_t align) noexcept {
			return (v + align - 1) & ~(align - 1);
		}

		Reservation reserve_and_commit(size_t requested_size) {
			Reservation res{};
			if (requested_size == 0) return res;

			size_t aligned_size = round_up(requested_size, m_alignment);
			if (aligned_size > m_capacity) return res;

			std::unique_lock<std::mutex> lock(m_mutex, std::defer_lock);
			if (m_thread_safe) lock.lock();

			if (m_capacity - m_used < aligned_size) return res;

			size_t space_at_head = m_capacity - m_head;

			if (m_head < m_tail) {
				space_at_head = m_tail - m_head;
			}

			if (space_at_head >= aligned_size) {
				res.count = 1;
				res.parts[0] = { m_head, aligned_size };
				res.total_advance = aligned_size;

				m_head = (m_head + aligned_size) % m_capacity;
				m_used += aligned_size;
				return res;
			}

			if (m_allow_wrap) {

				size_t space_at_zero = m_tail;


				if (space_at_zero >= aligned_size) {
					size_t padding = m_capacity - m_head;

					res.count = 1;
					res.parts[0] = { 0, aligned_size };

					res.total_advance = padding + aligned_size;

					m_head = aligned_size;

					m_used += (padding + aligned_size);

					return res;
				}
			}

			return res;
		}

		void release(size_t size_to_advance) {
			if (size_to_advance == 0) return;

			std::unique_lock<std::mutex> lock(m_mutex, std::defer_lock);
			if (m_thread_safe) lock.lock();

			assert(size_to_advance <= m_used && "Release amount exceeds used space!");

			m_tail = (m_tail + size_to_advance) % m_capacity;
			m_used -= size_to_advance;
		}

		void reset() {
			std::unique_lock<std::mutex> lock(m_mutex, std::defer_lock);
			if (m_thread_safe) lock.lock();
			m_head = m_tail = m_used = 0;
		}

	private:
		size_t m_capacity;
		size_t m_alignment;
		bool   m_allow_wrap;
		bool   m_thread_safe;

		size_t m_head; // next allocation begin
		size_t m_tail; // to release begin
		size_t m_used; // used bytesize -> used + padding

		mutable std::mutex m_mutex;
	};
}

#endif