#ifndef RING_BUFFER_H
#define RING_BUFFER_H
#include <array>
#include <mutex>
#include "NoCopyable.h"
namespace Render::Common {

    //FIFO
/*
 RingBufferAllocator
 - 不管理真实内存，只负责 offset/size 的分配元数据
 - reserve_and_commit : 原子的一步到位 reserve+commit（线程安全）
 - release(n) 要求按 tail 顺序释放
*/


    class RingBufferAllocator : public NonCopyable {
    public:
        struct Range {
            size_t offset;
            size_t size; // always multiple of m_alignment
        };

        struct Reservation {
            std::array<Range, 2> parts;
            int count = 0;          // 0 = fail, 1 = single, 2 = wrapped
            size_t total_advance = 0; // actual bytes reserved (multiple of m_alignment)
        };

        // capacity must be >0 and multiple of alignment
        // alignment must be 1 or power of two
        RingBufferAllocator(size_t capacity, size_t alignment, bool allow_split = true, bool thread_safe = false)
            : m_capacity(capacity), m_alignment(alignment),
            m_allow_split(allow_split), m_thread_safe(thread_safe),
            m_head(0), m_tail(0), m_used(0)
        {
            if (capacity == 0) throw std::invalid_argument("capacity must be > 0");
            if (alignment == 0) alignment = 1;
            if ((alignment & (alignment - 1)) != 0) throw std::invalid_argument("alignment must be power of two");
            if (capacity % alignment != 0) throw std::invalid_argument("capacity must be multiple of alignment");
        }

        RingBufferAllocator(const RingBufferAllocator&) = delete;
        RingBufferAllocator& operator=(const RingBufferAllocator&) = delete;
        RingBufferAllocator(RingBufferAllocator&&) = default;
        RingBufferAllocator& operator=(RingBufferAllocator&&) = default;

        size_t capacity() const noexcept { return m_capacity; }
        size_t alignment() const noexcept { return m_alignment; }
        size_t used() const noexcept { return m_used; }
        size_t free_space() const noexcept { return m_capacity - m_used; }
        size_t head_offset() const noexcept { return m_head; }
        size_t tail_offset() const noexcept { return m_tail; }

        // round up to multiple of alignment (alignment is pow2 => fast)
        static inline size_t round_up(size_t v, size_t align) noexcept {
            return (v + align - 1) & ~(align - 1);
        }

        // The only allocation API: atomic reserve+commit. Returns Reservation with parts sizes = rounded size(s).
        // Caller writes up to requested_size bytes into the returned regions (they may be larger due to rounding).
        Reservation reserve_and_commit(size_t requested_size) {
            if (requested_size == 0) return Reservation{};
            // round to alignment multiple
            size_t rounded = round_up(requested_size, m_alignment);
            if (rounded > m_capacity) return Reservation{}; // cannot satisfy even alone

            if (m_thread_safe) m_mutex.lock();
            Reservation res;
            do {
                size_t free = m_capacity - m_used;
                if (rounded > free) break;

                // contiguous free from head (in locked state)
                size_t contiguous = contiguous_from_head_locked();
                if (contiguous >= rounded) {
                    // single contiguous part
                    res.count = 1;
                    res.parts[0] = { m_head, rounded };
                }
                else {
                    // need split
                    if (!m_allow_split) break;
                    size_t first = contiguous; // could be 0
                    if (first == 0) {
                        // head is at used region or at capacity end; allocate from 0
                        size_t contiguous0 = contiguous_from_offset_locked(0);
                        if (contiguous0 < rounded) break; // should not happen because free >= rounded, but safe check
                        res.count = 1;
                        res.parts[0] = { 0, rounded };
                    }
                    else {
                        size_t second = rounded - first;
                        size_t contiguous0 = contiguous_from_offset_locked(0);
                        if (contiguous0 < second) break;
                        res.count = 2;
                        res.parts[0] = { m_head, first };
                        res.parts[1] = { 0, second };
                    }
                }

                // commit (advance head and used by rounded)
                m_head = (m_head + rounded) % m_capacity;
                m_used += rounded;
                res.total_advance = rounded;
            } while (false);

            if (m_thread_safe) m_mutex.unlock();
            return res;
        }

        // release must be multiple of alignment and <= used()
        void release(size_t n) {
            if (n == 0) return;
            if (n % m_alignment != 0) throw std::invalid_argument("release size must be multiple of alignment");
            if (m_thread_safe) m_mutex.lock();
            if (n > m_used) {
                if (m_thread_safe) m_mutex.unlock();
                throw std::invalid_argument("release more than used");
            }
            m_tail = (m_tail + n) % m_capacity;
            m_used -= n;
            if (m_thread_safe) m_mutex.unlock();
        }

        void reset() {
            if (m_thread_safe) {
                std::lock_guard<std::mutex> g(m_mutex);
                m_head = m_tail = m_used = 0;
            }
            else {
                m_head = m_tail = m_used = 0;
            }
        }

    private:
        // Must be called with lock held (if thread_safe true)
        size_t contiguous_from_head_locked() const noexcept {
            // if empty, contiguous := capacity - head
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

        // contiguous free from arbitrary offset (used when first==0 case)
        size_t contiguous_from_offset_locked(size_t offset) const noexcept {
            if (m_used == 0) {
                return m_capacity - offset;
            }
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
        bool   m_allow_split;
        bool   m_thread_safe;

        size_t m_head;
        size_t m_tail;
        size_t m_used;

        mutable std::mutex m_mutex;
    };
}

#endif