#ifndef SMALL_VECTOR_H
#define SMALL_VECTOR_H

#include <cstdint>
#include <cassert>
#include <type_traits>
#include <algorithm>
#include <memory>  
#include <new>  

template <typename T, size_t N = 1>
class SmallVector {
private:
    size_t m_size = 0;

    union {
        alignas(T) unsigned char m_inline_raw[N * sizeof(T)];
        T* m_heap;
    };

private:
    inline bool isHeap() const {
        return m_size > N;
    }

    inline T* inline_ptr() {
        return reinterpret_cast<T*>(m_inline_raw);
    }
    inline const T* inline_ptr() const {
        return reinterpret_cast<const T*>(m_inline_raw);
    }

public:
    inline SmallVector() : m_size(0) {}

    inline ~SmallVector() {
        std::destroy_n(data(), m_size);
        if (isHeap()) {
            ::operator delete(m_heap);
        }
    }

    inline SmallVector(const SmallVector& other) : m_size(other.m_size) {
        if (other.isHeap()) {
            m_heap = static_cast<T*>(::operator new(m_size * sizeof(T)));
            std::uninitialized_copy_n(other.m_heap, m_size, m_heap);
        }
        else {
            std::uninitialized_copy_n(other.inline_ptr(), m_size, inline_ptr());
        }
    }

    inline SmallVector& operator=(const SmallVector& other) {
        if (this == &other) return *this; 

        std::destroy_n(data(), m_size);
        if (isHeap()) {
            ::operator delete(m_heap);
        }

        m_size = other.m_size;
        if (other.isHeap()) {
            m_heap = static_cast<T*>(::operator new(m_size * sizeof(T)));
            std::uninitialized_copy_n(other.m_heap, m_size, m_heap);
        }
        else {
            std::uninitialized_copy_n(other.inline_ptr(), m_size, inline_ptr());
        }

        return *this;
    }

    inline SmallVector(SmallVector&& other) noexcept : m_size(other.m_size) {
        if (other.isHeap()) {
            this->m_heap = other.m_heap;
        }
        else {
            std::uninitialized_move_n(other.inline_ptr(), m_size, this->inline_ptr());
            std::destroy_n(other.inline_ptr(), m_size);
        }
        other.m_size = 0;
    }

    inline SmallVector& operator=(SmallVector&& other) noexcept {
        if (this == &other) return *this;

        std::destroy_n(data(), m_size);
        if (isHeap()) {
            ::operator delete(m_heap);
        }

        m_size = other.m_size;
        if (other.isHeap()) {
            this->m_heap = other.m_heap;
        }
        else {
            std::uninitialized_move_n(other.inline_ptr(), m_size, this->inline_ptr());
            std::destroy_n(other.inline_ptr(), m_size);
        }

        other.m_size = 0;
        return *this;
    }

    inline void resize(size_t new_size) {
        if (new_size == m_size) return;

        T* old_data = data();

        if (new_size <= N) {
            if (isHeap()) {
                std::uninitialized_move_n(old_data, new_size, inline_ptr());
                std::destroy_n(old_data, m_size);
                ::operator delete(old_data);
            }
            else {
                if (new_size > m_size) {
                    std::uninitialized_value_construct_n(inline_ptr() + m_size, new_size - m_size);
                }
                else {
                    std::destroy_n(inline_ptr() + new_size, m_size - new_size);
                }
            }
        }
        else {
            T* new_heap = static_cast<T*>(::operator new(new_size * sizeof(T)));
            size_t move_count = std::min(m_size, new_size);

            std::uninitialized_move_n(old_data, move_count, new_heap);

            if (new_size > m_size) {
                std::uninitialized_value_construct_n(new_heap + m_size, new_size - m_size);
            }

            std::destroy_n(old_data, m_size);
            if (isHeap()) {
                ::operator delete(old_data);
            }
            m_heap = new_heap;
        }
        m_size = new_size;
    }

    inline void resize(size_t new_size, const T& fill_value) {
        if (new_size == m_size) return;

        T* old_data = data();

        if (new_size <= N) {
            if (isHeap()) {
                std::uninitialized_move_n(old_data, new_size, inline_ptr());
                std::destroy_n(old_data, m_size);
                ::operator delete(old_data);
            }
            else {
                if (new_size > m_size) {
                    std::uninitialized_fill_n(inline_ptr() + m_size, new_size - m_size, fill_value);
                }
                else {
                    std::destroy_n(inline_ptr() + new_size, m_size - new_size);
                }
            }
        }
        else {
            T* new_heap = static_cast<T*>(::operator new(new_size * sizeof(T)));
            size_t move_count = std::min(m_size, new_size);

            std::uninitialized_move_n(old_data, move_count, new_heap);

            if (new_size > m_size) {
                std::uninitialized_fill_n(new_heap + m_size, new_size - m_size, fill_value);
            }

            std::destroy_n(old_data, m_size);
            if (isHeap()) {
                ::operator delete(old_data);
            }
            m_heap = new_heap;
        }
        m_size = new_size;
    }

    inline T& operator[](size_t idx) {
        assert(idx < m_size);
        return data()[idx];
    }

    inline const T& operator[](size_t idx) const {
        assert(idx < m_size);
        return data()[idx];
    }

    inline T* data() { return isHeap() ? m_heap : inline_ptr(); }
    inline const T* data() const { return isHeap() ? m_heap : inline_ptr(); }
    inline size_t size() const { return m_size; }
};

#endif // SMALL_VECTOR_H