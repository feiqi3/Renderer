#include "Renderer/MeshResourceManager.h"
#include "Renderer/RenderSystem.h"
#include  <vector>
namespace Render {

const Name& Mesh::GetTypeName() const
{
	static const Name sTypeName = Name("Mesh");
	return sTypeName;
}

ResourceMemory Mesh::GetMemory() const
{
	ResourceMemory memory{};
	memory.cpuMemory =
		sizeof(*this) +
		sizeof(rs_buffer) +
		sizeof(rs_buffer) +
		mSubMeshes.size() * sizeof(SubMesh) +
		mVertexLayout.size() * sizeof(MeshVertexAttribute);
	memory.gpuMemory = this->mVertexByteSize + this->mIndexByteSize;
	return memory;

}

}
