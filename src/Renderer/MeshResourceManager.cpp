#include "Renderer/RenderSystem.h"
#include "Renderer/MeshResourceManager.h"
#include "tiny_gltf.h"
#include  <vector>
namespace Render {

namespace {
	Mesh* getCubeMesh() {
		struct CubeVertex
		{
			float px, py, pz;   // Position
			float nx, ny, nz;   // Normal
			float u, v;         // TexCoord0
		};

		CubeVertex cubeVertices[24] = {
			// Front face (+Z)
			{-0.5f,-0.5f, 0.5f,0,0,1,0,0},
			{ 0.5f,-0.5f, 0.5f,0,0,1,1,0},
			{ 0.5f, 0.5f, 0.5f,0,0,1,1,1},
			{-0.5f, 0.5f, 0.5f,0,0,1,0,1},

			// Back face (-Z)
			{ 0.5f,-0.5f,-0.5f,0,0,-1,0,0},
			{-0.5f,-0.5f,-0.5f,0,0,-1,1,0},
			{-0.5f, 0.5f,-0.5f,0,0,-1,1,1},
			{ 0.5f, 0.5f,-0.5f,0,0,-1,0,1},

			// Left face (-X)
			{-0.5f,-0.5f,-0.5f,-1,0,0,0,0},
			{-0.5f,-0.5f, 0.5f,-1,0,0,1,0},
			{-0.5f, 0.5f, 0.5f,-1,0,0,1,1},
			{-0.5f, 0.5f,-0.5f,-1,0,0,0,1},

			// Right face (+X)
			{ 0.5f,-0.5f, 0.5f,1,0,0,0,0},
			{ 0.5f,-0.5f,-0.5f,1,0,0,1,0},
			{ 0.5f, 0.5f,-0.5f,1,0,0,1,1},
			{ 0.5f, 0.5f, 0.5f,1,0,0,0,1},

			// Top face (+Y)
			{-0.5f, 0.5f, 0.5f,0,1,0,0,0},
			{ 0.5f, 0.5f, 0.5f,0,1,0,1,0},
			{ 0.5f, 0.5f,-0.5f,0,1,0,1,1},
			{-0.5f, 0.5f,-0.5f,0,1,0,0,1},

			// Bottom face (-Y)
			{-0.5f,-0.5f,-0.5f,0,-1,0,0,0},
			{ 0.5f,-0.5f,-0.5f,0,-1,0,1,0},
			{ 0.5f,-0.5f, 0.5f,0,-1,0,1,1},
			{-0.5f,-0.5f, 0.5f,0,-1,0,0,1},
		};

		uint16_t cubeIndices[] =
		{
				0,  1,  2,   0,  2,  3,   // +X
				4,  5,  6,   4,  6,  7,   // -X
				8,  9, 10,   8, 10, 11,   // +Y
			12, 13, 14,  12, 14, 15,   // -Y
			16, 17, 18,  16, 18, 19,   // +Z
			20, 21, 22,  20, 22, 23,   // -Z
		};

		MeshData* cube = new MeshData(
			cubeVertices,
			sizeof(cubeVertices),
			24,
			cubeIndices,
			36,
			IndexType::Uint16
		);

		cube->setStride(sizeof(CubeVertex));
		auto ret = cube->toMeshResource();
		delete cube;
		return ret;
	}
} 
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

MeshResourceManager::MeshResourceManager()
{
}

const Name& MeshResourceManager::typeName() const {
	return Mesh::typeName();
}


void MeshResourceManager::createNecessaryPersistenceResources()
{
	auto cube = getCubeMesh();
	this->registerResource(Name("Builtin::Cube"), cube, ResourceLifetime::Persistent, nullptr);
}

Mesh* MeshResourceManager::loadImpl(const Name& id) {
	throw std::runtime_error("Not implement");
	return nullptr;
}

void MeshResourceManager::unloadImpl(Mesh* mesh)
{
	RenderSystem::instance()->destroyBuffer(mesh->mVertex);
	mesh->mVertex = 0;
	RenderSystem::instance()->destroyBuffer(mesh->mIndice);
	mesh->mIndice = 0;
	delete mesh;
}
size_t MeshData::getIndexByteSize()   const {
	size_t sizeOfIdxEle = this->getIndexType() == IndexType::Uint16 ? 2 : 4;
	return sizeOfIdxEle * this->getIndexCount();
}

rs_buffer* MeshData::updateToGPUVertex()
{
	auto rSys = RenderSystem::instance();
	BufferDesc desc{};
	desc.bufUsage = BufferType_Vertex;
	desc.queueType = QueueType_Graphics;
	desc.byteSize = this->getVertexByteSize();
	desc.mappable = false;
	auto vtxBuffer = rSys->createBuffer(getVertexData(),getVertexByteSize(), desc);
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
	mesh->mState = ResourceState::Loaded;
	return mesh;
}

}
