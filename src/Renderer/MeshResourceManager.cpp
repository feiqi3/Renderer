#include "Renderer/MeshResourceManager.h"
#include "Renderer/RenderSystem.h"
#include <stdexcept>
#include "Renderer/ModelVertex.h"
namespace Render {

	namespace {

		void GenerateSphere(float radius, uint32_t rings, uint32_t sectors,
			std::vector<StandardModelVertex>& outVertices,
			std::vector<uint32_t>& outIndices)
		{
			outVertices.clear();
			outIndices.clear();

			const float PI = 3.14159265358979323846f;


			//Vertex
			for (uint32_t r = 0; r <= rings; ++r) {
				float phi = PI * static_cast<float>(r) / static_cast<float>(rings); // 0 -> PI
				float sinPhi = std::sin(phi);
				float cosPhi = std::cos(phi);

				for (uint32_t s = 0; s <= sectors; ++s) {
					float theta = 2.0f * PI * static_cast<float>(s) / static_cast<float>(sectors); // 0 -> 2PI
					float sinTheta = std::sin(theta);
					float cosTheta = std::cos(theta);

					float nx = sinPhi * cosTheta;
					float ny = cosPhi;
					float nz = sinPhi * sinTheta;

					float px = radius * nx;
					float py = radius * ny;
					float pz = radius * nz;

					float tx = -sinTheta;
					float ty = 0.0f;
					float tz = cosTheta;
					float tw = 1.0f;

					float u = static_cast<float>(s) / static_cast<float>(sectors);
					float v = static_cast<float>(r) / static_cast<float>(rings);

					StandardModelVertex vertex = {
						{ px, py, pz },          // position
						{ nx, ny, nz },          // normal
						{ tx, ty, tz, tw },      // tangent
						{ u,  v },               // uv_0
						0xFFFFFFFF               // color
					};

					outVertices.push_back(vertex);
				}
			}

			//Index
			for (uint32_t r = 0; r < rings; ++r) {
				for (uint32_t s = 0; s < sectors; ++s) {
					uint32_t current = r * (sectors + 1) + s;
					uint32_t next = current + sectors + 1;

					outIndices.push_back(current);
					outIndices.push_back(next);
					outIndices.push_back(current + 1);

					outIndices.push_back(current + 1);
					outIndices.push_back(next);
					outIndices.push_back(next + 1);
				}
			}
		}

		Mesh* getSphereMesh() {
			std::vector<StandardModelVertex> outVtx;
			std::vector<uint32_t> indice;
			GenerateSphere(1, 10, 10, outVtx, indice);

			MeshData* sphere = new MeshData(
				outVtx.data(),
				outVtx.size() * sizeof(StandardModelVertex),
				4,
				indice.data(),
				indice.size(),
				IndexType::Uint32
			);
			sphere->addAttribute(VertexFormat::Float3, VertexSemantic::Position,offsetof(StandardModelVertex, position));
			sphere->addAttribute(VertexFormat::Float3, VertexSemantic::Normal  ,offsetof(StandardModelVertex, normal  ));
			sphere->addAttribute(VertexFormat::Float4, VertexSemantic::Tangent,offsetof(StandardModelVertex,  tangent));
			sphere->addAttribute(VertexFormat::Float2, VertexSemantic::TexCoord0,offsetof(StandardModelVertex, uv_0));
			sphere->addAttribute(VertexFormat::UByte4N, VertexSemantic::Color0,offsetof(StandardModelVertex, color_u8x4_pack));
			AxisAlignedBoundingBox aabb
				(vec3(-1,-1,-1),vec3(1,1,1))
				;
			sphere->addSubMesh(0, indice.size(), aabb, 0);
			sphere->setStride(sizeof(StandardModelVertex));

			auto ret = sphere->toMeshResource();
			delete sphere;
			return ret;
		}

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

		auto sphere = getSphereMesh();
		this->registerResource(Name("Builtin::Sphere"), sphere, ResourceLifetime::Persistent, nullptr);
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