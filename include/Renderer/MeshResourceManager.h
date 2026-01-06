#ifndef MESH_RESOURCE_MANAGER_H_
#define  MESH_RESOURCE_MANAGER_H_

#include "render_resource_def.h"
#include "common/ResourceHandler.h"
#include "common/ResourceManager.h"
namespace Render {
	enum class VertexSemantic : uint8_t
	{
		Position,
		Normal,
		Tangent,
		Bitangent,
		TexCoord0,
		TexCoord1,
		Color0,
		BoneIndices,
		BoneWeights,
	};

	struct rs_buffer;
	struct MeshVertexAttribute {
		VertexFormat		fmt;
		VertexSemantic		semantic;
		u32					offset;
	};
	class MeshData {
	public:
		MeshData(void* vertex, size_t dataSize, size_t vertexCount, void* indice, size_t indexNum, IndexType idxType);


		inline IndexType getIndexType()const { return mIndexType; }
		inline void setIndexType(IndexType t) { mIndexType = t; }

		inline void setStride(u32 s) { mStride = s; }
		inline u32 getStride()const { return mStride; }

		inline void addAttribute(VertexFormat fmt, VertexSemantic semantic, u32 offset) {
			mMeshVertexAttributes.push_back(
				MeshVertexAttribute{
					.fmt = fmt,
					.semantic = semantic,
					.offset = offset
				}
			);
		}
		inline const std::vector< MeshVertexAttribute>& getAttributes()const { return mMeshVertexAttributes; }
	private:
		void* pVertex;
		size_t mVertexCount;
		size_t mVertexSize;
		u32 mStride = 0;

		void* pIndice;
		size_t mIndexCount;
		IndexType mIndexType = IndexType::Uint16;

		std::vector< MeshVertexAttribute> mMeshVertexAttributes;
	};
	class Mesh {
	public:
	private:
		rs_buffer* mVertex = 0;
		rs_buffer* mIndice = 0;
	};
}
#endif