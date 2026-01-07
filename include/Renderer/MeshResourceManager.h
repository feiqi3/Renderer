#ifndef  MESH_RESOURCE_MANAGER_H_
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


	struct SubMesh
	{
		// base vertex for indexed draw
		int32_t  vertexOffset = 0;
        // index draw
        uint32_t indexOffset = 0;
        uint32_t indexCount = 0;
		uint32_t materialIndex = 0;
	};

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
        )
            : pVertex(vertex)
            , mVertexSize(vertexDataSize)
            , mVertexCount(vertexCount)
            , pIndice(indice)
            , mIndexCount(indexCount)
            , mIndexType(idxType)
        {
        }

        /* ================= Vertex ================= */

        inline void* getVertexData()       const { return pVertex; }
        inline size_t getVertexCount()      const { return mVertexCount; }
        inline size_t getVertexDataSize()   const { return mVertexSize; }

        inline void   setStride(u32 s) { mStride = s; }
        inline u32    getStride() const { return mStride; }

        /* ================= Index ================= */

        inline void* getIndexData() const { return pIndice; }
        inline size_t    getIndexCount()const { return mIndexCount; }
        inline IndexType getIndexType() const { return mIndexType; }
        inline void      setIndexType(IndexType t) { mIndexType = t; }

        /* ================= Attributes ================= */

        inline void addAttribute(VertexFormat fmt, VertexSemantic semantic, u32 offset) {
            mMeshVertexAttributes.push_back(
                MeshVertexAttribute{
                    .fmt = fmt,
                    .semantic = semantic,
                    .offset = offset
                }
            );
        }

        inline const std::vector<MeshVertexAttribute>&
            getAttributes() const { return mMeshVertexAttributes; }

        /* ================= SubMesh ================= */

        inline void addSubMesh(
            uint32_t indexOffset,
            uint32_t indexCount,
            uint32_t materialIndex = 0,
            int32_t  vertexOffset = 0
        ) {
            mSubMeshes.push_back(
                SubMesh{
                    .vertexOffset = vertexOffset,
                    .indexOffset = indexOffset,
                    .indexCount = indexCount,
                    .materialIndex = materialIndex
                }
            );
        }

        inline size_t getSubMeshCount() const {
            return mSubMeshes.size();
        }

        inline const SubMesh& getSubMesh(size_t i) const {
            return mSubMeshes[i];
        }

        inline const std::vector<SubMesh>&
            getSubMeshes() const {
            return mSubMeshes;
        }

    private:
        /* raw data */
        void* pVertex = nullptr;
        size_t mVertexSize = 0;
        size_t mVertexCount = 0;
        u32    mStride = 0;

        void* pIndice = nullptr;
        size_t mIndexCount = 0;
        IndexType mIndexType = IndexType::Uint16;

        /* layout */
        std::vector<MeshVertexAttribute> mMeshVertexAttributes;

        /* submeshes */
        std::vector<SubMesh> mSubMeshes;
    };

    class Mesh : public IResource {
    public:
        virtual const Name& GetTypeName() const override;
        virtual ResourceMemory GetMemory() const override;
    public:
        inline rs_buffer* getVertexBuffer() const { return mVertex; }
        inline rs_buffer* getIndexBuffer()  const { return mIndice; }

        inline IndexType getIndexType() const { return mIndexType; }

        inline size_t getSubMeshCount() const {
            return mSubMeshes.size();
        }

        inline const SubMesh& getSubMesh(size_t i) const {
            return mSubMeshes[i];
        }
        inline u32 getVertexCount() const {
            return mVertexCount;
        }

        inline u32 getVertexByteSize() const {
            return mVertexByteSize;
        }

        inline u32 getIndexCount() const {
            return mIndexCount;
        }

        inline u32 getIndexByteSize() const {
            return mIndexByteSize;
        }


        const std::vector<MeshVertexAttribute>& getVertexLayout()const {return mVertexLayout;}
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
    };
}
#endif