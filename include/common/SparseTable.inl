template <typename T>
template <typename U>
inline uint32_t SparseTable<T>::Allocate(U&& data) {
    if (denseData.size() >= maxCapacity) {
        return INVALID_INDEX;
    }

    uint32_t handleIndex = INVALID_INDEX;

    uint32_t currentIndex = lastSearchIndex + 1;
    if (currentIndex >= maxCapacity) currentIndex = 0;

    uint32_t startIndex = currentIndex;
    do {
        if (sparseIndices[currentIndex] == INVALID_INDEX) {
            handleIndex = currentIndex;
            break;
        }
        currentIndex++;
        if (currentIndex >= maxCapacity) currentIndex = 0;
    } while (currentIndex != startIndex);

    if (handleIndex == INVALID_INDEX) return INVALID_INDEX;

    lastSearchIndex = handleIndex;

    uint32_t denseIndex = static_cast<uint32_t>(denseData.size());
    denseData.push_back(std::forward<U>(data));
    denseToSparse.push_back(handleIndex);

    sparseIndices[handleIndex] = denseIndex;

    return handleIndex;
}

template <typename T>
inline void SparseTable<T>::Free(uint32_t handleIndex) {
    if (handleIndex >= maxCapacity || sparseIndices[handleIndex] == INVALID_INDEX) {
        return;
    }

    uint32_t denseIndexToPop = sparseIndices[handleIndex];
    uint32_t lastDenseIndex = static_cast<uint32_t>(denseData.size() - 1);

    if (denseIndexToPop != lastDenseIndex) {
        denseData[denseIndexToPop] = std::move(denseData[lastDenseIndex]);
        denseToSparse[denseIndexToPop] = denseToSparse[lastDenseIndex];

        uint32_t movedHandleIndex = denseToSparse[denseIndexToPop];
        sparseIndices[movedHandleIndex] = denseIndexToPop;
    }

    denseData.pop_back();
    denseToSparse.pop_back();

    sparseIndices[handleIndex] = INVALID_INDEX;
}

template <typename T>
inline T* SparseTable<T>::Get(uint32_t handleIndex) {
    if (handleIndex >= maxCapacity || sparseIndices[handleIndex] == INVALID_INDEX) {
        return nullptr;
    }
    return &denseData[sparseIndices[handleIndex]];
}

template <typename T>
inline const T* SparseTable<T>::Get(uint32_t handleIndex) const {
    if (handleIndex >= maxCapacity || sparseIndices[handleIndex] == INVALID_INDEX) {
        return nullptr;
    }
    return &denseData[sparseIndices[handleIndex]];
}