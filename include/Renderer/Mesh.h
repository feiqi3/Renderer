#ifndef MESH_H_
#define MESH_H_

#include "render_resource_def.h"
#include "common/ResourceHandler.h"
#include "function/AABB.h"
#include <vector>

namespace Render {

	class Mesh; // Forward declaration

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
		VertexFormat        fmt;
		VertexSemantic      semantic;
		u32                 offset;
	};

	struct SubMesh
	{
		int32_t  vertexOffset = 0; // base vertex for indexed draw
		uint32_t indexOffset = 0;  // index draw
		uint32_t indexCount = 0;
		AxisAlignedBoundingBox aabb = {};
	};

	// ==========================================
	// Class: MeshData (Helper for building mesh)
	// ==========================================
	class MeshData {
	public:
		MeshData() = default;

		MeshData(
			void* vertex,
			size_t vertexDataSize,
			size_t vertexCount,
			void* indice,
			size_t indexCount,
			IndexType idxType
		);

		/* ================= Vertex ================= */
		inline void* getVertexData()     const { return pVertex; }
		inline size_t getVertexCount()    const { return mVertexCount; }
		inline size_t getVertexByteSize() const { return mVertexByteSize; }

		inline void   setStride(u32 s) { mStride = s; }
		inline u32    getStride() const { return mStride; }

		/* ================= Index ================= */
		inline void* getIndexData() const { return pIndice; }
		inline size_t    getIndexCount()const { return mIndexCount; }
		inline IndexType getIndexType() const { return mIndexType; }
		inline void      setIndexType(IndexType t) { mIndexType = t; }
		size_t           getIndexByteSize() const;

		/* ================= Attributes ================= */
		void addAttribute(VertexFormat fmt, VertexSemantic semantic, u32 offset);
		inline const std::vector<MeshVertexAttribute>& getAttributes() const { return mMeshVertexAttributes; }

		/* ================= SubMesh ================= */
		void addSubMesh(uint32_t indexOffset, uint32_t indexCount, int32_t vertexOffset = 0);
		inline void setSubMeshes(std::vector<SubMesh>& submesh) { mSubMeshes = submesh; }

		inline size_t getSubMeshCount() const { return mSubMeshes.size(); }
		inline const SubMesh& getSubMesh(size_t i) const { return mSubMeshes[i]; }
		inline const std::vector<SubMesh>& getSubMeshes() const { return mSubMeshes; }

		// Operations
		rs_buffer* updateToGPUVertex();
		rs_buffer* updateToGPUIndice();
		Mesh* toMeshResource();

	private:
		/* raw data */
		void* pVertex = nullptr;
		size_t mVertexByteSize = 0;
		size_t mVertexCount = 0;
		u32    mStride = 0;

		void* pIndice = nullptr;
		size_t    mIndexCount = 0;
		IndexType mIndexType = IndexType::Uint16;

		/* layout */
		std::vector<MeshVertexAttribute> mMeshVertexAttributes;

		/* submeshes */
		std::vector<SubMesh> mSubMeshes;
	};

	// ==========================================
	// Class: Mesh (The actual Resource)
	// ==========================================
	class Mesh : public IResource {
	public:
		static const Name& typeName();
		virtual const Name& getTypeName() const override;
		virtual ResourceMemory getMemory() const override;

	public:
		inline rs_buffer* getVertexBuffer() const { return mVertex; }
		inline rs_buffer* getIndexBuffer()  const { return mIndice; }
		inline IndexType  getIndexType()    const { return mIndexType; }

		inline size_t getSubMeshCount() const { return mSubMeshes.size(); }
		inline const SubMesh& getSubMesh(size_t i) const { return mSubMeshes[i]; }

		inline u32 getVertexCount() const { return mVertexCount; }
		inline u32 getVertexByteSize() const { return mVertexByteSize; }

		inline u32 getIndexCount() const { return mIndexCount; }
		inline u32 getIndexByteSize() const { return mIndexByteSize; }

		inline const std::vector<MeshVertexAttribute>& getVertexLayout() const { return mVertexLayout; }

	private:
		rs_buffer* mVertex = nullptr;
		rs_buffer* mIndice = nullptr;

		/* vertex stats */
		u32 mVertexCount = 0;
		u32 mVertexByteSize = 0;

		/* index stats */
		u32 mIndexCount = 0;
		u32 mIndexByteSize = 0;

		IndexType mIndexType = IndexType::Uint16;

		std::vector<SubMesh> mSubMeshes;
		std::vector<MeshVertexAttribute> mVertexLayout;

		friend class MeshResourceManager;
		friend class MeshData;
	};
	using MeshPtr = ResourceHandle<Mesh>;
}

#endif // MESH_H_