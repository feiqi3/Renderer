#ifndef RING_BUFFER_H
#define RING_BUFFER_H
#include <array>
#include <mutex>
#include "NoCopyable.h"
#include <cassert>
namespace Render::Common {

    //FIFO
    class RingBufferAllocator : public NonCopyable {
    public:
        struct Range { size_t offset; size_t size; }; // size multiple of alignment

        struct Reservation {
            std::array<Range, 1> parts; // only single contiguous part in this version
            int count = 0;              // 0 = fail, 1 = single
            size_t total_advance = 0;   // how many bytes head advanced on the ring (includes skipped tail-fragment if wrapped to 0)
        };

        RingBufferAllocator(size_t capacity, size_t alignment, bool allow_wrap_to_zero = true, bool thread_safe = false)
            : m_capacity(capacity),
            m_alignment(alignment == 0 ? 1 : alignment),
            m_allow_wrap(allow_wrap_to_zero),
            m_thread_safe(thread_safe),
            m_head(0), m_tail(0), m_used(0)
        {
            if (m_capacity == 0) throw std::invalid_argument("capacity must be > 0");
            if ((m_alignment & (m_alignment - 1)) != 0) throw std::invalid_argument("alignment must be power of two");
            if (m_capacity % m_alignment != 0) throw std::invalid_argument("capacity must be multiple of alignment");
        }

        RingBufferAllocator(const RingBufferAllocator&) = delete;
        RingBufferAllocator& operator=(const RingBufferAllocator&) = delete;

        size_t capacity() const noexcept { return m_capacity; }
        size_t alignment() const noexcept { return m_alignment; }
        size_t used() const noexcept { return m_used; }
        size_t free_space() const noexcept { return m_capacity - m_used; }
        size_t head_offset() const noexcept { return m_head; }
        size_t tail_offset() const noexcept { return m_tail; }

        static inline size_t round_up(size_t v, size_t align) noexcept {
            return (v + align - 1) & ~(align - 1);
        }

        // Only contiguous allocations. Will try head -> if not enough and wrapping allowed, try offset 0.
        Reservation reserve_and_commit(size_t requested_size) {
            Reservation res{};
            if (requested_size == 0) return res;

            size_t rounded = round_up(requested_size, m_alignment);
            if (rounded > m_capacity) return res;

            std::unique_lock<std::mutex> lock(m_mutex, std::defer_lock);
            if (m_thread_safe) lock.lock();

            // quick capacity check
            size_t free_total = m_capacity - m_used;
            if (rounded > free_total) return res; // cannot satisfy

            // contiguous free from head
            size_t contiguous_head = contiguous_from_head_locked();

            if (contiguous_head >= rounded) {
                // allocate at head
                res.count = 1;
                res.parts[0] = { m_head, rounded };

                // commit
                m_head = (m_head + rounded) % m_capacity;
                m_used += rounded;
                res.total_advance = rounded;
                return res;
            }

            // not enough contiguous at head: try allocate from offset 0 if allowed
            if (!m_allow_wrap) return res;

            size_t contiguous0 = contiguous_from_offset_locked(0);
            if (contiguous0 < rounded) {
                // cannot satisfy as single contiguous block
                return res;
            }

            // We will allocate at offset 0. Per your requirement, the head->capacity tail fragment
            // (contiguous_head) becomes unusable now and must be counted as used.
            size_t old_head = m_head;
            size_t tail_fragment = contiguous_head; // how many bytes from old_head..capacity are lost
            if (tail_fragment > 0) {
                m_used += tail_fragment;
            }

            // allocate at offset 0
            res.count = 1;
            res.parts[0] = { 0, rounded };
            m_head = rounded % m_capacity;
            m_used += rounded;

            // total_advance = skipped_tail_fragment + allocated_size (how far head advanced along ring)
            res.total_advance = tail_fragment + rounded;

            // final sanity
            if (m_used > m_capacity) 
            {
                assert(0);
                m_used = m_capacity; 
            } // defensive (shouldn't happen due to checks)

            return res;
        }

        // release must be multiple of alignment and <= used()
        // FIFO: caller must release in allocation order. We check size but we don't track per-reservation metadata here.
        void release(size_t n) {
            if (n == 0) return;
            if (n % m_alignment != 0) throw std::invalid_argument("release size must be multiple of alignment");

            std::unique_lock<std::mutex> lock(m_mutex, std::defer_lock);
            if (m_thread_safe) lock.lock();

            if (n > m_used) throw std::invalid_argument("release more than used");

            m_tail = (m_tail + n) % m_capacity;
            m_used -= n;
        }

        void reset() {
            std::unique_lock<std::mutex> lock(m_mutex, std::defer_lock);
            if (m_thread_safe) lock.lock();
            m_head = m_tail = m_used = 0;
        }

    private:
        // must be called with lock held (if thread_safe)
        size_t contiguous_from_head_locked() const noexcept {
            if (m_used == 0) {
                return m_capacity - m_head;
            }
            if (m_head >= m_tail) {
                // used in [tail, head)
                return m_capacity - m_head; // free from head to end
            }
            else {
                // used spans end: free is [head, tail)
                return m_tail - m_head;
            }
        }

        // contiguous free from arbitrary offset
        size_t contiguous_from_offset_locked(size_t offset) const noexcept {
            if (m_used == 0) return m_capacity - offset;
            if (m_head >= m_tail) {
                if (offset >= m_head) return m_capacity - offset;
                if (offset < m_tail) return m_tail - offset;
                return 0;
            }
            else {
                if (offset >= m_head && offset < m_tail) return m_tail - offset;
                return 0;
            }
        }

    private:
        size_t m_capacity;
        size_t m_alignment;
        bool   m_allow_wrap;
        bool   m_thread_safe;

        size_t m_head;
        size_t m_tail;
        size_t m_used;

        mutable std::mutex m_mutex;
    };
}

#endif