#pragma once

#include <vector>
#include <cstdint>
#include <utility>
#include "common/SmallVector.h"

namespace Render {

    template <typename T>
    class SparseTable {
    private:
        SmallVector<uint32_t> sparseIndices;

        std::vector<T> denseData;
        std::vector<uint32_t> denseToSparse;

        uint32_t maxCapacity;
        uint32_t lastSearchIndex = 0;

    public:
        static constexpr uint32_t INVALID_INDEX = 0xFFFFFFFF;

        SparseTable(uint32_t maxLimit) : maxCapacity(maxLimit) {
            sparseIndices.resize(maxCapacity, INVALID_INDEX);
            denseData.reserve(256);
            denseToSparse.reserve(256);
        }

        template <typename U>
        uint32_t Allocate(U&& data);

        void Free(uint32_t handleIndex);

        T* Get(uint32_t handleIndex);
        const T* Get(uint32_t handleIndex) const;

        uint32_t Size() const { return static_cast<uint32_t>(denseData.size()); }

        T* Data() { return denseData.empty() ? nullptr : denseData.data(); }
        const T* Data() const { return denseData.empty() ? nullptr : denseData.data(); }

        typename std::vector<T>::iterator begin() { return denseData.begin(); }
        typename std::vector<T>::iterator end() { return denseData.end(); }
        typename std::vector<T>::const_iterator begin() const { return denseData.begin(); }
        typename std::vector<T>::const_iterator end() const { return denseData.end(); }
    };

#include "SparseTable.inl"
}
