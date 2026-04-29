#include "render_resource.h"
#include "common/BindlessIndexingTable.h"
namespace Render {

	BindlessIndexingTable::BindlessIndexingTable(uint32_t maxLength)
	{
		this->maxCapacity = maxLength;
		this->sparseIndices.resize(maxLength, INVALID_BINDLESS_INDEX);
	}

	rs_resource* BindlessIndexingTable::get(uint32_t idx)
	{
		if (idx >= sparseIndices.size())return nullptr;
		uint32_t denseIdx = sparseIndices[idx];
		if (denseIdx == INVALID_INDEX) return nullptr;
		return denseData[denseIdx].resource;
	}

	uint32_t BindlessIndexingTable::Allocate(uint64_t frame, rs_resource* data)
    {
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
		BindlessSlot slot{};
		slot.resource					= data;
		slot.resource->bindlessIndex	= handleIndex;
		slot.lastUsedFrame				= frame;
		slot.bindlessIndex				= handleIndex;
		slot.usedRef					= 1;
		denseData.push_back(slot);

		sparseIndices[handleIndex] = denseIndex;

		return handleIndex;
    }

	void BindlessIndexingTable::Free(uint32_t handleIndex)
	{
		if (handleIndex >= maxCapacity || sparseIndices[handleIndex] == INVALID_INDEX) {
			return;
		}

		uint32_t denseIndexToPop = sparseIndices[handleIndex];
		
		rs_resource* toEraseResource = denseData[denseIndexToPop].resource;
		if (toEraseResource) {
			toEraseResource->bindlessIndex = INVALID_INDEX;
		}

		uint32_t lastDenseIndex = static_cast<uint32_t>(denseData.size() - 1);

		if (denseIndexToPop != lastDenseIndex) {
			uint32_t HandlerToBeSwapedSparseIndex = denseData[lastDenseIndex].bindlessIndex;
			//Invalid original resource
			//Swap invalid resource slot with the last one.
			std::swap(denseData[denseIndexToPop],denseData[lastDenseIndex]);
			//Redirect sparse index.
			if(HandlerToBeSwapedSparseIndex != INVALID_INDEX)
				sparseIndices[HandlerToBeSwapedSparseIndex] = denseIndexToPop;
		}

		denseData.pop_back();
		//Clear slot
		sparseIndices[handleIndex] = INVALID_INDEX;
	}

	uint32_t BindlessIndexingTable::IncRef(uint32_t handleIndex)
	{
		if (handleIndex >= sparseIndices.size()) {
			assert(false);
			return 0;
		}

		uint32_t realDenseIndex = sparseIndices[handleIndex];
		if (realDenseIndex >= denseData.size()){
			assert(false);
			return 0;
		}
		return ++denseData[realDenseIndex].usedRef;

	}

	uint32_t BindlessIndexingTable::DeRef(uint32_t handleIndex)
	{
		if (handleIndex >= sparseIndices.size()) {
			assert(false);
			return 0;
		}

		uint32_t realDenseIndex = sparseIndices[handleIndex];
		if (realDenseIndex >= denseData.size()) {
			assert(false);
			return 0;
		}
		auto ret = --denseData[realDenseIndex].usedRef;
		if (ret == 0) {
			Free(handleIndex);
		}
		return ret;
	}

}