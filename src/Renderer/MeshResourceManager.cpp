#include "Renderer/MeshResourceManager.h"
#include "Renderer/RenderSystem.h"
#include <stdexcept>

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
				{-0.5f,-0.5f, 0.5f,0,0,1,0,0}, { 0.5f,-0.5f, 0.5f,0,0,1,1,0},
				{ 0.5f, 0.5f, 0.5f,0,0,1,1,1}, {-0.5f, 0.5f, 0.5f,0,0,1,0,1},
				// Back face (-Z)
				{ 0.5f,-0.5f,-0.5f,0,0,-1,0,0}, {-0.5f,-0.5f,-0.5f,0,0,-1,1,0},
				{-0.5f, 0.5f,-0.5f,0,0,-1,1,1}, { 0.5f, 0.5f,-0.5f,0,0,-1,0,1},
				// Left face (-X)
				{-0.5f,-0.5f,-0.5f,-1,0,0,0,0}, {-0.5f,-0.5f, 0.5f,-1,0,0,1,0},
				{-0.5f, 0.5f, 0.5f,-1,0,0,1,1}, {-0.5f, 0.5f,-0.5f,-1,0,0,0,1},
				// Right face (+X)
				{ 0.5f,-0.5f, 0.5f,1,0,0,0,0}, { 0.5f,-0.5f,-0.5f,1,0,0,1,0},
				{ 0.5f, 0.5f,-0.5f,1,0,0,1,1}, { 0.5f, 0.5f, 0.5f,1,0,0,0,1},
				// Top face (+Y)
				{-0.5f, 0.5f, 0.5f,0,1,0,0,0}, { 0.5f, 0.5f, 0.5f,0,1,0,1,0},
				{ 0.5f, 0.5f,-0.5f,0,1,0,1,1}, {-0.5f, 0.5f,-0.5f,0,1,0,0,1},
				// Bottom face (-Y)
				{-0.5f,-0.5f,-0.5f,0,-1,0,0,0}, { 0.5f,-0.5f,-0.5f,0,-1,0,1,0},
				{ 0.5f,-0.5f, 0.5f,0,-1,0,1,1}, {-0.5f,-0.5f, 0.5f,0,-1,0,0,1},
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
			cube->addSubMesh(0, 36, 0);
			cube->setStride(sizeof(CubeVertex));
			auto ret = cube->toMeshResource();
			delete cube;
			return ret;
		}
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
		if (mesh) {
			RenderSystem::instance()->destroyBuffer(mesh->mVertex);
			mesh->mVertex = nullptr;
			RenderSystem::instance()->destroyBuffer(mesh->mIndice);
			mesh->mIndice = nullptr;
			delete mesh;
		}
	}

} // namespace Render