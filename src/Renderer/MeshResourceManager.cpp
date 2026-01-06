#include "Renderer/MeshResourceManager.h"

#include  <vector>
namespace Render {


MeshData::MeshData(void* vertex, size_t dataSize, size_t vertexCount, void* indice, size_t indexNum, IndexType idxType)
	:pVertex(vertex),mVertexCount(vertexCount), mVertexSize(dataSize), mStride(dataSize / vertexCount), pIndice(indice), mIndexCount(indexNum), mIndexType(idxType)
{

}

}
