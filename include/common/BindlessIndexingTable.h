#ifndef BINDLESS_INDEXING_TABLE_H_
#define BINDLESS_INDEXING_TABLE_H_
#include <cstdint>
#include <vector>
#include "SmallVector.h"
namespace Render {
	struct rs_resource;

	struct BindlessSlot {
		rs_resource* resource = nullptr;
		uint64_t lastUsedFrame = 0;
		uint32_t usedRef = 0;
		UniformType type = UniformType::Count;
		uint32_t bindlessIndex = INVALID_BINDLESS_INDEX;
		bool resourceStateNeedTransit = false;
	};
	//OPTIMIZE: add a free list to record.
	class BindlessIndexingTable {
	public:
		const static uint32_t INVALID_INDEX = INVALID_BINDLESS_INDEX;
		//Index in the bindless array.
		SmallVector<uint32_t> sparseIndices;
		//Real data.
		std::vector<BindlessSlot> denseData;
		std::vector<uint32_t> denseIndexToTransit;
		uint32_t maxCapacity;
		uint32_t lastSearchIndex = 0;

	public:
		BindlessIndexingTable(uint32_t maxLength);
		rs_resource*	get(uint32_t);
		uint32_t		Allocate(uint64_t frame,rs_resource* data);
		void			Free(uint32_t handleIndex);
		uint32_t		IncRef(uint32_t handleIndex);
		uint32_t		DeRef(uint32_t handleIndex);
		void			ResourceStateMark(uint32_t handleIndex);
		void			ClearResourceStateMarked();
		void			FreeAll();
	};
}

#endif