#include "Renderer/MeshResourceManager.h"
#include "Renderer/RenderSystem.h"
#include <stdexcept>
#include "Renderer/ModelVertex.h"
namespace Render {

	namespace {

		Mesh* getQuadMesh() {
			struct QuadVertex
			{
				float px, py, pz;   // Position
				float nx, ny, nz;   // Normal
				float u, v;         // TexCoord0
			};

			StandardModelVertex quadVertices[4] = {
				// --- Front Face (+Z) ---
				// position               normal              tangent                 uv_0         color
				{{-0.5f, -0.5f,  0.0f},  { 0.0f, 0.0f, 1.0f}, { 1.0f, 0.0f, 0.0f, 1.0f}, {0.0f, 0.0f}, 0}, // 0: Bottom-Left
				{{ 0.5f, -0.5f,  0.0f},  { 0.0f, 0.0f, 1.0f}, { 1.0f, 0.0f, 0.0f, 1.0f}, {1.0f, 0.0f}, 0}, // 1: Bottom-Right
				{{ 0.5f,  0.5f,  0.0f},  { 0.0f, 0.0f, 1.0f}, { 1.0f, 0.0f, 0.0f, 1.0f}, {1.0f, 1.0f}, 0}, // 2: Top-Right
				{{-0.5f,  0.5f,  0.0f},  { 0.0f, 0.0f, 1.0f}, { 1.0f, 0.0f, 0.0f, 1.0f}, {0.0f, 1.0f}, 0}  // 3: Top-Left
			};

			std::vector<uint32_t> quadIndices;
			quadIndices.reserve(6);

			quadIndices.push_back(0);
			quadIndices.push_back(1);
			quadIndices.push_back(2);
			quadIndices.push_back(2);
			quadIndices.push_back(3);
			quadIndices.push_back(0);

			MeshData* quad = new MeshData(
				quadVertices,
				sizeof(quadVertices),
				4,
				quadIndices.data(),
				6,
				IndexType::Uint32
			);

			AxisAlignedBoundingBox aabb;
			for (int i = 0; i < sizeof(quadVertices) / sizeof(StandardModelVertex); ++i) {
				aabb.expand(quadVertices[i].position);
			}

			quad->addSubMesh(0, 6, aabb, 0);

			quad->setStride(sizeof(StandardModelVertex));

			auto ret = quad->toMeshResource();
			delete quad;
			return ret;
		}

		Mesh* getCubeMesh() {
			struct CubeVertex
			{
				float px, py, pz;   // Position
				float nx, ny, nz;   // Normal
				float u, v;         // TexCoord0
			};

			StandardModelVertex cubeVertices[24] = {
				// --- Front Face (+Z) ---
				// position               normal             tangent                 uv_0        color
				{{-0.5f, -0.5f,  0.5f},  { 0.0f, 0.0f, 1.0f}, { 1.0f, 0.0f, 0.0f, 1.0f}, {0.0f, 0.0f}, 0}, // 0: Bottom-Left
				{{ 0.5f, -0.5f,  0.5f},  { 0.0f, 0.0f, 1.0f}, { 1.0f, 0.0f, 0.0f, 1.0f}, {1.0f, 0.0f}, 0}, // 1: Bottom-Right
				{{ 0.5f,  0.5f,  0.5f},  { 0.0f, 0.0f, 1.0f}, { 1.0f, 0.0f, 0.0f, 1.0f}, {1.0f, 1.0f}, 0}, // 2: Top-Right
				{{-0.5f,  0.5f,  0.5f},  { 0.0f, 0.0f, 1.0f}, { 1.0f, 0.0f, 0.0f, 1.0f}, {0.0f, 1.0f}, 0}, // 3: Top-Left

				// --- Back Face (-Z) ---
				{{ 0.5f, -0.5f, -0.5f},  { 0.0f, 0.0f,-1.0f}, {-1.0f, 0.0f, 0.0f, 1.0f}, {0.0f, 0.0f}, 0}, // 4
				{{-0.5f, -0.5f, -0.5f},  { 0.0f, 0.0f,-1.0f}, {-1.0f, 0.0f, 0.0f, 1.0f}, {1.0f, 0.0f}, 0}, // 5
				{{-0.5f,  0.5f, -0.5f},  { 0.0f, 0.0f,-1.0f}, {-1.0f, 0.0f, 0.0f, 1.0f}, {1.0f, 1.0f}, 0}, // 6
				{{ 0.5f,  0.5f, -0.5f},  { 0.0f, 0.0f,-1.0f}, {-1.0f, 0.0f, 0.0f, 1.0f}, {0.0f, 1.0f}, 0}, // 7

				// --- Left Face (-X) ---
				{{-0.5f, -0.5f, -0.5f},  {-1.0f, 0.0f, 0.0f}, { 0.0f, 0.0f, 1.0f, 1.0f}, {0.0f, 0.0f}, 0}, // 8
				{{-0.5f, -0.5f,  0.5f},  {-1.0f, 0.0f, 0.0f}, { 0.0f, 0.0f, 1.0f, 1.0f}, {1.0f, 0.0f}, 0}, // 9
				{{-0.5f,  0.5f,  0.5f},  {-1.0f, 0.0f, 0.0f}, { 0.0f, 0.0f, 1.0f, 1.0f}, {1.0f, 1.0f}, 0}, // 10
				{{-0.5f,  0.5f, -0.5f},  {-1.0f, 0.0f, 0.0f}, { 0.0f, 0.0f, 1.0f, 1.0f}, {0.0f, 1.0f}, 0}, // 11

				// --- Right Face (+X) ---
				{{ 0.5f, -0.5f,  0.5f},  { 1.0f, 0.0f, 0.0f}, { 0.0f, 0.0f,-1.0f, 1.0f}, {0.0f, 0.0f}, 0}, // 12
				{{ 0.5f, -0.5f, -0.5f},  { 1.0f, 0.0f, 0.0f}, { 0.0f, 0.0f,-1.0f, 1.0f}, {1.0f, 0.0f}, 0}, // 13
				{{ 0.5f,  0.5f, -0.5f},  { 1.0f, 0.0f, 0.0f}, { 0.0f, 0.0f,-1.0f, 1.0f}, {1.0f, 1.0f}, 0}, // 14
				{{ 0.5f,  0.5f,  0.5f},  { 1.0f, 0.0f, 0.0f}, { 0.0f, 0.0f,-1.0f, 1.0f}, {0.0f, 1.0f}, 0}, // 15

				// --- Top Face (+Y) ---
				{{-0.5f,  0.5f,  0.5f},  { 0.0f, 1.0f, 0.0f}, { 1.0f, 0.0f, 0.0f, 1.0f}, {0.0f, 0.0f}, 0}, // 16
				{{ 0.5f,  0.5f,  0.5f},  { 0.0f, 1.0f, 0.0f}, { 1.0f, 0.0f, 0.0f, 1.0f}, {1.0f, 0.0f}, 0}, // 17
				{{ 0.5f,  0.5f, -0.5f},  { 0.0f, 1.0f, 0.0f}, { 1.0f, 0.0f, 0.0f, 1.0f}, {1.0f, 1.0f}, 0}, // 18
				{{-0.5f,  0.5f, -0.5f},  { 0.0f, 1.0f, 0.0f}, { 1.0f, 0.0f, 0.0f, 1.0f}, {0.0f, 1.0f}, 0}, // 19

				// --- Bottom Face (-Y) ---
				{{-0.5f, -0.5f, -0.5f},  { 0.0f,-1.0f, 0.0f}, { 1.0f, 0.0f, 0.0f, 1.0f}, {0.0f, 0.0f}, 0}, // 20
				{{ 0.5f, -0.5f, -0.5f},  { 0.0f,-1.0f, 0.0f}, { 1.0f, 0.0f, 0.0f, 1.0f}, {1.0f, 0.0f}, 0}, // 21
				{{ 0.5f, -0.5f,  0.5f},  { 0.0f,-1.0f, 0.0f}, { 1.0f, 0.0f, 0.0f, 1.0f}, {1.0f, 1.0f}, 0}, // 22
				{{-0.5f, -0.5f,  0.5f},  { 0.0f,-1.0f, 0.0f}, { 1.0f, 0.0f, 0.0f, 1.0f}, {0.0f, 1.0f}, 0}  // 23
			};
			std::vector<uint32_t> cubeIndices;
			cubeIndices.reserve(36);
			for (uint32_t i = 0; i < 6; ++i) {
				uint32_t baseIndex = i * 4;
				cubeIndices.push_back(baseIndex + 0);
				cubeIndices.push_back(baseIndex + 1);
				cubeIndices.push_back(baseIndex + 2);
				cubeIndices.push_back(baseIndex + 2);
				cubeIndices.push_back(baseIndex + 3);
				cubeIndices.push_back(baseIndex + 0);
			}

			MeshData* cube = new MeshData(
				cubeVertices,
				sizeof(cubeVertices),
				24,
				cubeIndices.data(),
				36,
				IndexType::Uint32
			);
			AxisAlignedBoundingBox aabb;
			for (int i = 0;i < sizeof(cubeVertices) / sizeof(StandardModelVertex);++i) {
				aabb.expand(cubeVertices[i].position);
			}
			cube->addSubMesh(0, 36, aabb, 0);
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
		auto quad = getQuadMesh();
		this->registerResource(Name("Builtin::Quad"), quad, ResourceLifetime::Persistent, nullptr);
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