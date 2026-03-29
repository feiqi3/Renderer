#include "Renderer/Mesh.h"
#include "Renderer/RenderSystem.h" 
#include <stdexcept>

namespace Render {

	// ==========================================
	// MeshData Implementation
	// ==========================================
	static const SubMesh cEmptySubMesh = {};

	MeshData::MeshData(
		void* vertex,
		size_t vertexDataSize,
		size_t vertexCount,
		void* indice,
		size_t indexCount,
		IndexType idxType
	)
		: pVertex(vertex)
		, mVertexByteSize(vertexDataSize)
		, mVertexCount(vertexCount)
		, pIndice(indice)
		, mIndexCount(indexCount)
		, mIndexType(idxType)
	{
	}

	size_t MeshData::getIndexByteSize() const {
		size_t sizeOfIdxEle = this->getIndexType() == IndexType::Uint16 ? 2 : 4;
		return sizeOfIdxEle * this->getIndexCount();
	}

	void MeshData::addAttribute(VertexFormat fmt, VertexSemantic semantic, u32 offset) {
		mMeshVertexAttributes.push_back(
			MeshVertexAttribute{
				.fmt = fmt,
				.semantic = semantic,
				.offset = offset
			}
		);
	}

	void MeshData::addSubMesh(uint32_t indexOffset, uint32_t indexCount, int32_t vertexOffset) {
		mSubMeshes.push_back(
			SubMesh{
				.vertexOffset = vertexOffset,
				.indexOffset = indexOffset,
				.indexCount = indexCount,
			}
			);
	}

	rs_buffer* MeshData::updateToGPUVertex()
	{
		auto rSys = RenderSystem::instance();
		BufferDesc desc{};
		desc.bufUsage = BufferType_Vertex;
		desc.queueType = QueueType_Graphics;
		desc.byteSize = this->getVertexByteSize();
		desc.mappable = false;
		auto vtxBuffer = rSys->createBuffer(getVertexData(), getVertexByteSize(), desc);
		return vtxBuffer;
	}

	rs_buffer* MeshData::updateToGPUIndice()
	{
		auto rSys = RenderSystem::instance();
		BufferDesc desc{};
		desc.queueType = QueueType_Graphics;
		desc.mappable = false;
		desc.bufUsage = BufferType_Index;
		desc.byteSize = this->getIndexByteSize();
		auto idxBuffer = rSys->createBuffer(getIndexData(), getIndexByteSize(), desc);
		return idxBuffer;
	}

	Mesh* MeshData::toMeshResource()
	{
		Mesh* mesh = new Mesh();
		mesh->mVertex = this->updateToGPUVertex();
		mesh->mVertexByteSize = this->getVertexByteSize();
		mesh->mVertexCount = this->getVertexCount();

		mesh->mIndice = this->updateToGPUIndice();
		mesh->mIndexCount = this->getIndexCount();
		mesh->mIndexType = this->getIndexType();
		mesh->mIndexByteSize = this->getIndexByteSize();

		mesh->mVertexLayout = this->getAttributes();
		mesh->mSubMeshes = this->mSubMeshes;

		mesh->mState = ResourceLoadState::Loaded;
		return mesh;
	}

	// ==========================================
	// Mesh Implementation
	// ==========================================

	const Name& Mesh::typeName() {
		static const Name sTypeName = Name("Mesh");
		return sTypeName;
	}

	const Name& Mesh::getTypeName() const
	{
		return typeName();
	}

	ResourceMemory Mesh::getMemory() const
	{
		ResourceMemory memory{};
		memory.cpuMemory =
			sizeof(*this) +
			sizeof(rs_buffer) + 
			sizeof(rs_buffer) +
			mSubMeshes.capacity() * sizeof(SubMesh) +
			mVertexLayout.capacity() * sizeof(MeshVertexAttribute);

		memory.gpuMemory = this->mVertexByteSize + this->mIndexByteSize;
		return memory;
	}
}