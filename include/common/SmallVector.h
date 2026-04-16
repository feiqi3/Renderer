#ifndef SMALL_VECTOR_H
#define SMALL_VECTOR_H
#include <cstdint>
#include <cstring>
#include <cassert>
#include <type_traits>

template <typename T, size_t N = 1>
class SmallVector {
	static_assert(std::is_trivial_v<T>, "SmallVector is optimized for trivial types (like pointers)!");

private:
	size_t m_size = 0;
	bool m_isHeap = false;

	union {
		T m_inline[N];
		T* m_heap;
	};

public:
	inline SmallVector() : m_size(0), m_isHeap(false) {
		std::memset(m_inline, 0, sizeof(T) * N);
	}

	inline ~SmallVector() {
		if (m_isHeap) {
			delete[] m_heap;
		}
	}

	inline SmallVector(const SmallVector&) = delete;
	inline SmallVector& operator=(const SmallVector&) = delete;

	inline SmallVector(SmallVector&& other) noexcept {
		m_size = other.m_size;
		m_isHeap = other.m_isHeap;
		if (m_isHeap) {
			m_heap = other.m_heap;
			other.m_heap = nullptr;
		}
		else {
			std::memcpy(m_inline, other.m_inline, sizeof(T) * N);
		}
		other.m_size = 0;
		other.m_isHeap = false;
	}

	inline SmallVector& operator=(SmallVector&& other) noexcept {
		if (this != &other) {
			if (m_isHeap) delete[] m_heap;

			m_size = other.m_size;
			m_isHeap = other.m_isHeap;
			if (m_isHeap) {
				m_heap = other.m_heap;
				other.m_heap = nullptr;
			}
			else {
				std::memcpy(m_inline, other.m_inline, sizeof(T) * N);
			}
			other.m_size = 0;
			other.m_isHeap = false;
		}
		return *this;
	}

	inline void resize(size_t new_size) {
		if (new_size <= N) {
			if (m_isHeap) {
				delete[] m_heap;
				m_isHeap = false;
			}
			m_size = new_size;
			std::memset(m_inline, 0, sizeof(T) * N);
		}
		else {
			T* new_heap = new T[new_size]{};
			if (m_isHeap) {
				std::memcpy(new_heap, m_heap, std::min(m_size, new_size) * sizeof(T));
				delete[] m_heap;
			}
			else {
				std::memcpy(new_heap, m_inline, std::min(m_size, new_size) * sizeof(T));
			}
			m_heap = new_heap;
			m_isHeap = true;
			m_size = new_size;
		}
	}

	inline inline T& operator[](size_t idx) {
		assert(idx < m_size);
		return m_isHeap ? m_heap[idx] : m_inline[idx];
	}

	inline T* data() { return m_isHeap ? m_heap : m_inline; }
	inline size_t size() const { return m_size; }
};
#endif SMALL_VECTOR_H