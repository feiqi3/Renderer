#include "Renderer/GltfLoader.h"
#include "platform/FileSystem/FileSystem.h"
#include "common/ResourceSystem.h"
#include <iostream>
#include "Renderer/ModelVertex.h"
#include "Renderer/Mesh.h"
#include "function/Scene.h"
#include "function/Object.h"
#include "Components/PBRRenderComponent.h"
#include "Renderer/Materials/PBRMaterial.h"
#include "Renderer/MaterialTemplateManager.h"
#include "common/ResourceSystem.h"
#include "Renderer/MaterialManager.h"
#include "Renderer/SamplerResourceManager.h"
#include "Renderer/ModelResourceManager.h"
#include "Renderer/EnginePass.h"
#include <vector>
#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "tiny_gltf.h"


namespace {

    Render::SamplerDesc fromGltfSamplerToSamplerDesc(const Render::GLTFSampler& sampler) {
        using namespace Render;
        
        Render::SamplerDesc desc{};
        desc.addressU = sampler.addressS;
        desc.addressV = sampler.addressT;
        desc.addressW = AddressMode::Repeat;
        desc.minFilter = sampler.minFilter;
        desc.magFilter = sampler.magFilter;
        return desc;
	}

    enum class TypeOfVertexElement {
        Vec4,
        Vec3,
        Vec2,
        Uint8X4,t
    };

    static Render::AddressMode fromGltfToAddressMode(int s) {
        switch (s) {
        case TINYGLTF_TEXTURE_WRAP_REPEAT:
            return Render::AddressMode::Repeat;
        case TINYGLTF_TEXTURE_WRAP_CLAMP_TO_EDGE:
            return Render::AddressMode::ClampToEdge;
        case TINYGLTF_TEXTURE_WRAP_MIRRORED_REPEAT:
            return Render::AddressMode::MirroredRepeat;
        }
        return Render::AddressMode::ClampToEdge;
    }

    static Render::Filter fromGltfToFilter(int s) {
        switch (s) {
        case TINYGLTF_TEXTURE_FILTER_NEAREST:
        case TINYGLTF_TEXTURE_FILTER_NEAREST_MIPMAP_NEAREST:
        case TINYGLTF_TEXTURE_FILTER_NEAREST_MIPMAP_LINEAR:
            return Render::Filter::Nearest;
        case TINYGLTF_TEXTURE_FILTER_LINEAR:
        case TINYGLTF_TEXTURE_FILTER_LINEAR_MIPMAP_NEAREST:
        case TINYGLTF_TEXTURE_FILTER_LINEAR_MIPMAP_LINEAR:
            return Render::Filter::Linear;
        }
        return Render::Filter::Nearest;
    }

    static int NumComponentsInType(int type) {
        // tinygltf defines TINYGLTF_TYPE_* constants
        switch (type) {
        case TINYGLTF_TYPE_SCALAR: return 1;
        case TINYGLTF_TYPE_VEC2:   return 2;
        case TINYGLTF_TYPE_VEC3:   return 3;
        case TINYGLTF_TYPE_VEC4:   return 4;
        case TINYGLTF_TYPE_MAT2:   return 4;
        case TINYGLTF_TYPE_MAT3:   return 9;
        case TINYGLTF_TYPE_MAT4:   return 16;
        default: return 1;
        }
    }

    static int ComponentByteSize(int componentType) {
        switch (componentType) {
        case TINYGLTF_COMPONENT_TYPE_BYTE:           return 1;
        case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE:  return 1;
        case TINYGLTF_COMPONENT_TYPE_SHORT:          return 2;
        case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT: return 2;
        case TINYGLTF_COMPONENT_TYPE_INT:            return 4;
        case TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT:   return 4;
        case TINYGLTF_COMPONENT_TYPE_FLOAT:          return 4;
        default: return 0;
        }
    }

	template <typename T>
	inline float ReadGltfFloat(const uint8_t* ptr, bool normalized) {
		T val = *reinterpret_cast<const T*>(ptr);
		return static_cast<float>(val);
	}

	template <> inline float ReadGltfFloat<int8_t>(const uint8_t* ptr, bool normalized) {
		int8_t v = *reinterpret_cast<const int8_t*>(ptr);
		if (normalized) return std::max(-1.0f, static_cast<float>(v) / 127.0f);
		return static_cast<float>(v);
	}
	template <> inline float ReadGltfFloat<uint8_t>(const uint8_t* ptr, bool normalized) {
		uint8_t v = *reinterpret_cast<const uint8_t*>(ptr);
		return normalized ? (static_cast<float>(v) / 255.0f) : static_cast<float>(v);
	}
	template <> inline float ReadGltfFloat<int16_t>(const uint8_t* ptr, bool normalized) {
		int16_t v = *reinterpret_cast<const int16_t*>(ptr);
		if (normalized) return std::max(-1.0f, static_cast<float>(v) / 32767.0f);
		return static_cast<float>(v);
	}
	template <> inline float ReadGltfFloat<uint16_t>(const uint8_t* ptr, bool normalized) {
		uint16_t v = *reinterpret_cast<const uint16_t*>(ptr);
		return normalized ? (static_cast<float>(v) / 65535.0f) : static_cast<float>(v);
	}
	template <> inline float ReadGltfFloat<int32_t>(const uint8_t* ptr, bool normalized) {
		int32_t v = *reinterpret_cast<const int32_t*>(ptr);
		if (normalized) return std::max(-1.0f, static_cast<float>(v) / 2147483647.0f);
		return static_cast<float>(v);
	}
	template <> inline float ReadGltfFloat<uint32_t>(const uint8_t* ptr, bool normalized) {
		uint32_t v = *reinterpret_cast<const uint32_t*>(ptr);
		return normalized ? (static_cast<float>(v) / 4294967295.0f) : static_cast<float>(v);
	}

	template <typename T>
	inline uint8_t ReadGltfUint8(const uint8_t* ptr, bool normalized) {
		float f = ReadGltfFloat<T>(ptr, normalized);
		f = std::max(0.0f, std::min(1.0f, f));
		return static_cast<uint8_t>(std::round(f * 255.0f));
	}
	template <> inline uint8_t ReadGltfUint8<int8_t>(const uint8_t* ptr, bool normalized) {
		int8_t v = *reinterpret_cast<const int8_t*>(ptr);
		if (normalized) {
			float f = std::max(-1.0f, std::min(1.0f, static_cast<float>(v) / 127.0f));
			return static_cast<uint8_t>(std::round((f * 0.5f + 0.5f) * 255.0f));
		}
		return static_cast<uint8_t>(std::max(0, std::min(255, static_cast<int>(v))));
	}
	template <> inline uint8_t ReadGltfUint8<int16_t>(const uint8_t* ptr, bool normalized) {
		int16_t v = *reinterpret_cast<const int16_t*>(ptr);
		if (normalized) {
			float f = std::max(-1.0f, std::min(1.0f, static_cast<float>(v) / 32767.0f));
			return static_cast<uint8_t>(std::round((f * 0.5f + 0.5f) * 255.0f));
		}
		return static_cast<uint8_t>(std::max(0, std::min(255, static_cast<int>(v))));
	}
	template <> inline uint8_t ReadGltfUint8<uint8_t>(const uint8_t* ptr, bool normalized) {
		return *reinterpret_cast<const uint8_t*>(ptr);
	}

	inline float DispatchReadFloat(const uint8_t* ptr, int compType, bool normalized) {
		switch (compType) {
		case TINYGLTF_COMPONENT_TYPE_BYTE:           return ReadGltfFloat<int8_t>(ptr, normalized);
		case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE:  return ReadGltfFloat<uint8_t>(ptr, normalized);
		case TINYGLTF_COMPONENT_TYPE_SHORT:          return ReadGltfFloat<int16_t>(ptr, normalized);
		case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT: return ReadGltfFloat<uint16_t>(ptr, normalized);
		case TINYGLTF_COMPONENT_TYPE_INT:            return ReadGltfFloat<int32_t>(ptr, normalized);
		case TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT:   return ReadGltfFloat<uint32_t>(ptr, normalized);
		case TINYGLTF_COMPONENT_TYPE_FLOAT:          return ReadGltfFloat<float>(ptr, normalized);
		default: return 0.0f;
		}
	}

	inline uint8_t DispatchReadUint8(const uint8_t* ptr, int compType, bool normalized) {
		switch (compType) {
		case TINYGLTF_COMPONENT_TYPE_BYTE:           return ReadGltfUint8<int8_t>(ptr, normalized);
		case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE:  return ReadGltfUint8<uint8_t>(ptr, normalized);
		case TINYGLTF_COMPONENT_TYPE_SHORT:          return ReadGltfUint8<int16_t>(ptr, normalized);
		case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT: return ReadGltfUint8<uint16_t>(ptr, normalized);
		case TINYGLTF_COMPONENT_TYPE_INT:            return ReadGltfUint8<int32_t>(ptr, normalized);
		case TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT:   return ReadGltfUint8<uint32_t>(ptr, normalized);
		case TINYGLTF_COMPONENT_TYPE_FLOAT:          return ReadGltfUint8<float>(ptr, normalized);
		default: return 0;
		}
	}

	class GltfAttributeReader {
	public:
		const uint8_t* basePtr = nullptr;
		size_t stride = 0;
		int compType = 0;
		int numComps = 0;
		bool normalized = false;
		bool valid = false;

		GltfAttributeReader(const tinygltf::Model& model, const tinygltf::Primitive& prim, const char* attrName) {
			auto it = prim.attributes.find(attrName);
			if (it == prim.attributes.end()) return;

			const tinygltf::Accessor& acc = model.accessors[it->second];
			const tinygltf::BufferView& view = model.bufferViews[acc.bufferView];
			const tinygltf::Buffer& buffer = model.buffers[view.buffer];

			if (buffer.data.empty()) return;

			basePtr = buffer.data.data() + view.byteOffset + acc.byteOffset;
			stride = view.byteStride ? view.byteStride : (ComponentByteSize(acc.componentType) * NumComponentsInType(acc.type));
			compType = acc.componentType;
			numComps = NumComponentsInType(acc.type);
			normalized = acc.normalized;
			valid = true;
		}

		void getFloats(size_t vtxIdx, float* out, int maxOutComps) const {
			if (!valid) return;
			const uint8_t* ptr = basePtr + vtxIdx * stride;
			int compsToRead = std::min(numComps, maxOutComps);

			for (int i = 0; i < compsToRead; ++i) {
				const uint8_t* compPtr = ptr + i * ComponentByteSize(compType);
				out[i] = DispatchReadFloat(compPtr, compType, normalized);
			}

			if (maxOutComps == 4 && numComps == 3) out[3] = 1.0f;
			else if (maxOutComps == 4 && numComps == 1) { out[1] = out[2] = 0.0f; out[3] = 1.0f; }
		}

		void getUint8s(size_t vtxIdx, uint8_t* out, int maxOutComps) const {
			if (!valid) return;
			const uint8_t* ptr = basePtr + vtxIdx * stride;
			int compsToRead = std::min(numComps, maxOutComps);

			for (int i = 0; i < compsToRead; ++i) {
				const uint8_t* compPtr = ptr + i * ComponentByteSize(compType);
				out[i] = DispatchReadUint8(compPtr, compType, normalized);
			}
		}
	};

    Render::MaterialTemplatePtr getPBRMaterialTemplate(const Render::GLTFMaterial* material) {
        using namespace Render;
        auto materialTemplateMgr = MaterialTemplateManager::instance();
        std::string materialName = "PBRMaterialTemplate";
        std::string vsMarco = "";
        std::string psMarco = "";
        switch (material->alphaMode) {
            case GLTFAlphaMode::Opaque:
                materialName += "_Opaque";
				break;
            case GLTFAlphaMode::Mask:
				materialName += "_Mask";
                psMarco += "#define ALPHA_TEST\n";
                break;
            case GLTFAlphaMode::Blend:
				materialName += "_Blend";
                psMarco += "#define ALPHA_BLEND\n";
                break;
        }
        Name tpltName = Name(materialName);
		auto pbrTplt =  materialTemplateMgr->getMaterialTemplate(tpltName);
        if (!pbrTplt) {
            //Try create one
            RenderState state{};
            if (material->alphaMode == GLTFAlphaMode::Blend) {
                BlendState glassBlendForMainRT;

                glassBlendForMainRT.blendEnable = true;

                glassBlendForMainRT.srcColorBlend = BlendFactor::SrcAlpha;        
                glassBlendForMainRT.dstColorBlend = BlendFactor::OneMinusSrcAlpha; 
                glassBlendForMainRT.colorBlendOp = BlendOp::Add;                 

                glassBlendForMainRT.srcAlphaBlend = BlendFactor::One;             
                glassBlendForMainRT.dstAlphaBlend = BlendFactor::Zero;            
                glassBlendForMainRT.alphaBlendOp = BlendOp::Add;
                state.blendStates.push_back(glassBlendForMainRT);
            }
            VertexInputDescription vtxID{};
            InputBufferBinding binding{};
			binding.stride = sizeof(StandardModelVertex);
            vtxID.bindings.push_back(binding);
            int offset = 0;
            InputAttribute ia{};
            ia.binding = 0;
            ia.location = 0;
            ia.format = VertexFormat::Float3;
            ia.offset = 0;
			vtxID.attributes.push_back(ia);
			ia.offset += sizeof(float) * 3;

            ia.location = 1;
            vtxID.attributes.push_back(ia);
            ia.offset += sizeof(float) * 3;

            ia.location = 2;
            ia.format = VertexFormat::Float2;
			vtxID.attributes.push_back(ia);
			offset += sizeof(float) * 2;

            ia.location = 3;
            ia.format = VertexFormat::Float2;
            vtxID.attributes.push_back(ia);
            offset += sizeof(float) * 2;

            ia.location = 4;
            ia.format = VertexFormat::Uint4;
            vtxID.attributes.push_back(ia);
            offset += sizeof(uint32_t);

            auto tplt = materialTemplateMgr->createMaterialTemplate(tpltName, { {ShaderStage::Vertex,""},{ShaderStage::Fragment,psMarco} }, state, vtxID);
            tplt->createMaterialPass(RenderSystem::instance()->getRenderPass(PassName::MainCameraPass));
        }
        else {
            return pbrTplt;
        }
    }

	bool packPrimitiveToStandardVertices(
		const tinygltf::Model& model,
		const tinygltf::Mesh& mesh,
		std::vector<Render::StandardModelVertex>& outVertices,
		std::vector<uint32_t>& outIndice,
		std::vector<Render::SubMesh>& outSubmeshes,
		std::vector<int>& outMaterialIndex)
	{
		size_t totalCntVtx = 0;
		size_t totalIndice = 0;
        for (const auto& prim : mesh.primitives) {
            auto itPos = prim.attributes.find("POSITION");
            if (itPos == prim.attributes.end()) return false;
            totalCntVtx += static_cast<size_t>(model.accessors[itPos->second].count);
            if (prim.indices < 0) {
                //Generate by hand
                totalIndice += static_cast<size_t>(model.accessors[itPos->second].count);
            }
            else {
                totalIndice += static_cast<size_t>(model.accessors[prim.indices].count);
            }
        }

		outVertices.resize(totalCntVtx);
		outIndice.resize(totalIndice, 0);

		size_t baseVertexOffset = 0;
		size_t baseIndiceOffset = 0;

		for (const auto& prim : mesh.primitives) {
			auto itPos = prim.attributes.find("POSITION");
			size_t vertexCount = static_cast<size_t>(model.accessors[itPos->second].count);
			if (vertexCount == 0) return false;

			GltfAttributeReader posReader(model, prim, "POSITION");
			GltfAttributeReader normReader(model, prim, "NORMAL");
			GltfAttributeReader uv0Reader(model, prim, "TEXCOORD_0");
			GltfAttributeReader uv1Reader(model, prim, "TEXCOORD_1");
			GltfAttributeReader colReader(model, prim, "COLOR_0");

			for (size_t i = 0; i < vertexCount; ++i) {
				Render::StandardModelVertex vtx{};
				vtx.color_u8x4_pack = 0xFFFFFFFFu; 

				if (posReader.valid) {
					float pos[3] = { 0.f, 0.f, 0.f };
					posReader.getFloats(i, pos, 3);
					memcpy(&vtx.position, pos, sizeof(float) * 3);
				}
				if (normReader.valid) {
					float norm[3] = { 0.f, 0.f, 0.f };
					normReader.getFloats(i, norm, 3);
					memcpy(&vtx.normal, norm, sizeof(float) * 3);
				}
				if (uv0Reader.valid) {
					float uv0[2] = { 0.f, 0.f };
					uv0Reader.getFloats(i, uv0, 2);
					memcpy(&vtx.uv_0, uv0, sizeof(float) * 2);
				}
				if (uv1Reader.valid) {
					float uv1[2] = { 0.f, 0.f };
					uv1Reader.getFloats(i, uv1, 2);
					memcpy(&vtx.uv_1, uv1, sizeof(float) * 2);
				}
				if (colReader.valid) {
					uint8_t col[4] = { 255, 255, 255, 255 };
					colReader.getUint8s(i, col, 4);
					memcpy(&vtx.color_u8x4_pack, col, sizeof(uint8_t) * 4);
				}

				outVertices[baseVertexOffset + i] = vtx;
			}

            if (prim.indices >= 0) {

                const tinygltf::Accessor& indiceAcc = model.accessors[prim.indices];
                const tinygltf::BufferView& view = model.bufferViews[indiceAcc.bufferView];
                const tinygltf::Buffer& buffer = model.buffers[view.buffer];
                const uint8_t* src = buffer.data.data() + view.byteOffset + indiceAcc.byteOffset;

                for (size_t i = 0; i < indiceAcc.count; ++i) {
                    uint32_t idx = 0;
                    switch (indiceAcc.componentType) {
                    case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE:
                        idx = static_cast<uint32_t>(reinterpret_cast<const uint8_t*>(src)[i]);
                        break;
                    case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT:
                        idx = static_cast<uint32_t>(reinterpret_cast<const uint16_t*>(src)[i]);
                        break;
                    case TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT:
                        idx = reinterpret_cast<const uint32_t*>(src)[i];
                        break;
                    default:
                        return false;
                    }
                    outIndice[baseIndiceOffset + i] = idx + static_cast<uint32_t>(baseVertexOffset);
                }

                Render::SubMesh sub;
                //bake into indice
                //sub.vertexOffset = static_cast<int32_t>(baseVertexOffset);
                sub.indexOffset = static_cast<uint32_t>(baseIndiceOffset);
                sub.indexCount = static_cast<uint32_t>(indiceAcc.count);
                outSubmeshes.push_back(sub);
				baseIndiceOffset += indiceAcc.count;
            }
            else {
                //No indice condition
				auto itPos = prim.attributes.find("POSITION");
                if (itPos == prim.attributes.end()) {
                    return false;
                }
                auto& posAcc = model.accessors[itPos->second];
                for (int i = 0;i < posAcc.count; ++i) {
					outIndice[baseIndiceOffset + i] = i + static_cast<uint32_t>(baseVertexOffset);
                }
				Render::SubMesh sub;
				//bake into indice
				//sub.vertexOffset = static_cast<int32_t>(baseVertexOffset);
				sub.indexOffset = static_cast<uint32_t>(baseIndiceOffset);
				sub.indexCount = static_cast<uint32_t>(posAcc.count);
				outSubmeshes.push_back(sub);
				baseIndiceOffset += posAcc.count;
            }

			outMaterialIndex.push_back(prim.material);
			baseVertexOffset += vertexCount;
		}
		return true;
	}

}

namespace Render {

    namespace {
    
        void addSocketForSkeleton(const Name& name, GLTFSkeleton& out, const tinygltf::Model& model, const tinygltf::Skin& skin, std::unordered_map<int, int>& nodeToJoint) {
            for (auto& nodeIdx : skin.joints) {
                const auto& node = model.nodes[nodeIdx];
                int jointIdx = nodeToJoint[nodeIdx];
                auto& joint = out.joints[jointIdx];
                for (auto& child : node.children) {
                    auto it = nodeToJoint.find(child);
                    if (it == nodeToJoint.end()) {
                        GLTFJoint j{};
                        const auto& n = model.nodes[child];
                        // ---- bind pose local TRS ----
                        if (n.translation.size() == 3) {
                            j.translation = vec3(
                                float(n.translation[0]),
                                float(n.translation[1]),
                                float(n.translation[2]));
                        }

                        if (n.rotation.size() == 4) {
                            // glTF: [x, y, z, w]
                            j.rotation = vec4(
                                float(n.rotation[0]),
                                float(n.rotation[1]),
                                float(n.rotation[2]),
                                float(n.rotation[3]));
                        }

                        if (n.scale.size() == 3) {
                            j.scale = vec3(
                                float(n.scale[0]),
                                float(n.scale[1]),
                                float(n.scale[2]));
                        }
                        j.isJoint = false;
                        j.name = n.name;
                        j.parent = jointIdx;
                        j.children.clear();
                        out.joints.push_back(j);
                        joint.children.push_back(out.joints.size() - 1);
                    }
                }
            }
        }
    
    }

    class GLTFLoaderPrivate {
    public:
        tinygltf::TinyGLTF loader;
        bool            getGLTFModel(const std::string& modelName, GLTFModel& out, const tinygltf::Model& model);
        bool            getGLTFScene(GLTFScene& out, const tinygltf::Scene& scene);
        bool            getSkeleton(const Name& name, GLTFSkeleton& out, const tinygltf::Model& model, const tinygltf::Skin& skin);
        bool            getMesh(const Name& name, GLTFMesh& out, const tinygltf::Model& model, const tinygltf::Mesh& mesh);
        bool            getMaterial(const Name& name, GLTFMaterial& out, const tinygltf::Model& model, const tinygltf::Material& mat);
        bool            getTexture(const Name& name, GLTFTexture& out, const tinygltf::Model& model, const tinygltf::Texture& texture);
        bool            getSampler(const Name& name, GLTFSampler& out, const tinygltf::Model& model, const tinygltf::Sampler& sampler);
        bool            getNode(const Name& name, GLTFNode& out, const tinygltf::Model& model, const tinygltf::Node& node);
    
		MaterialPtr     createPBRMaterialFromGLTFMaterial(GLTFModel* model, const GLTFMaterial& gltfMat);
    
    };


    namespace {
        bool IsFileExist(const std::string& str, void* userPtr) {
            using namespace Platform;
            auto fileExists = FileSystem::instance()->openFileStream(str);
            if (fileExists != nullptr) {
                return true;
            }
            return false;
        }
        std::string ExpandFilePath(const std::string& path, void* userPTr) {
            //See tinyGLTF::ExpandFilePath
            return path;
        }
        bool ReadWholeFile(std::vector<unsigned char>* out, std::string* err,
            const std::string& filepath, void*) {
            using namespace Platform;
            auto filestream = FileSystem::instance()->openFileStream(filepath);

            if (!out)return false;

            if (filestream == nullptr) {
                if (err) {
                    *err += std::string("FileNotExists: " + filepath);
                }
                return false;
            }
            auto fileSize = filestream->getSize();
            if (fileSize == 0) {
                if (err) {
                    *err += std::string("FileInvalid: " + filepath);
                }
                return false;
            }
            out->resize(fileSize);
            auto readSize = filestream->read(out->data(), out->size());
            assert(readSize == fileSize);
            return true;
        }

        bool WriteWholeFile(std::string* err, const std::string& filepath,
            const std::vector<unsigned char>& contents, void*) {
            using namespace Platform;
            auto filestream = FileSystem::instance()->createFileStream(filepath);
            if (!filestream || !(filestream->getState() & FileAccess::WRITE)) {
                if (err) {
                    *err += "Cannot create file in path: " + filepath + "; May caused by FileSystem limits.";
                }
                return false;
            }
            filestream->write(contents.data(), contents.size());
            return true;
        }

        bool GetFileSizeInBytes(size_t* filesize_out, std::string* err,
            const std::string& filepath, void*) {
            using namespace Platform;
            auto filestream = FileSystem::instance()->openFileStream(filepath);
            if (filestream == nullptr) {
                if (err) {
                    *err += std::string("FileNotExists: " + filepath);
                }
                return false;
            }
            *filesize_out = filestream->getSize();
            return true;
        }
    };


    GLTFLoader::GLTFLoader()
    {
        mDp = new GLTFLoaderPrivate;
        tinygltf::FsCallbacks filestreamCB{
        .FileExists = &IsFileExist,
        .ExpandFilePath = &ExpandFilePath,
        .ReadWholeFile = &ReadWholeFile,
        .WriteWholeFile = &WriteWholeFile,
        .GetFileSizeInBytes = &GetFileSizeInBytes
        };
        mDp->loader.SetFsCallbacks(filestreamCB);
    }

    GLTFLoader::~GLTFLoader()
    {
        delete mDp;
        mDp = nullptr;
    }

    GLTFModel* Render::GLTFLoader::createFromFilePath(const std::string& path)
    {
        auto& loader = mDp->loader;
        tinygltf::Model model;
        std::string err;
        std::string warn;

        bool loadRes = false;
        if (path.ends_with(".glb")) {
            loadRes = loader.LoadBinaryFromFile(&model, &err, &warn, path);
        }
        else if (path.ends_with(".gltf")) {
            loadRes = loader.LoadASCIIFromFile(&model, &err, &warn, path);
        }
        if (!warn.empty()) {
            printf("GLTFLOADER Warn: %s\n", warn.c_str());
        }

        if (!err.empty()) {
            printf("GLTFLOADER Err: %s\n", err.c_str());
        }
        if (!loadRes) {
            return nullptr;
        }

        GLTFModel* innerModel = new GLTFModel;
        bool getModelSuccess = mDp->getGLTFModel(path, *innerModel, model);
        return innerModel;
    }

    Model* GLTFLoader::gltfModelToEngimeModel(GLTFModel* gltfModel)
    {
		Model* model = new Model;
        //set mesh and ptr
        for (auto& mesh : gltfModel->meshes) {
            std::vector<MaterialPtr> materials{};
            for (auto& matid : mesh.materialIdx) {
				auto matPtr = mDp->createPBRMaterialFromGLTFMaterial(gltfModel, gltfModel->materials[matid]);
                materials.push_back(matPtr);
            }
            model->addMesh(mesh.mesh, materials);
        }
        return model;
    }

	Object* GLTFLoader::toEngineSceneNode(Scene* scene, GLTFModel* model)
	{
		if (!model || model->scenes.empty()) {
			return nullptr;
		}

		Object* rootObj = scene->createObject(model->modelName.c_str());
		assert(rootObj != nullptr);

		std::function<void(int, Object*)> processNode = [&](int nodeIndex, Object* parentObj) {
			if (nodeIndex < 0 || nodeIndex >= model->nodes.size()) return;

			const GLTFNode& node = model->nodes[nodeIndex];

			std::string objName = node.name.empty() ? ("Node_" + std::to_string(nodeIndex)) : node.name;
			Object* currentObj = scene->createObject(objName.c_str());

			currentObj->setLocalPosition(node.translation);
			currentObj->setLocalRotation(fromEulerAngles(node.rotation));
			currentObj->setLocalScale(node.scale);

			if (node.meshIndex >= 0 && node.meshIndex < model->meshes.size()) {
				const auto& mesh = model->meshes[node.meshIndex];
				auto renderComp = currentObj->addComponent<Render::PBRRenderComponent>();
				renderComp->setMesh(mesh.mesh);

				int submeshIdx = 0;
				for (int materialIDX : mesh.materialIdx) {
					if (materialIDX >= 0 && materialIDX < model->materials.size()) {
						auto matPtr = mDp->createPBRMaterialFromGLTFMaterial(model, model->materials[materialIDX]);
						renderComp->setMaterial(submeshIdx, matPtr);
					}
					submeshIdx++;
				}
			}

			if (parentObj) {
				parentObj->addChild(currentObj);
			}

			for (int childIndex : node.children) {
				processNode(childIndex, currentObj);
			}
			};

		const GLTFScene& defaultScene = model->scenes[0];
		for (int rootNodeIndex : defaultScene.nodes) {
			processNode(rootNodeIndex, rootObj);
		}

		return rootObj;
	}
    bool GLTFLoaderPrivate::getGLTFModel(const std::string& modelName, GLTFModel& out, const tinygltf::Model& model)
    {
        out.scenes.clear();
        out.nodes.clear();
        out.meshes.clear();
        out.materials.clear();
        out.textures.clear();
        out.samplers.clear();
        out.skeletons.clear();

        Name modelNamePrefix = Name(modelName+":");

        std::unordered_map<int, int> gltfNodeToEngine;
        std::unordered_map<int, int> gltfMeshToEngine;
        std::unordered_map<int, int> gltfMatToEngine;
        std::unordered_map<int, int> gltfTexToEngine;
        std::unordered_map<int, int> gltfSamplerToEngine;
        std::unordered_map<int, int> gltfSkinToEngine;

        std::function<bool(int)> processSampler;
        std::function<bool(int)> processTexture;
        std::function<bool(int)> processMaterial;
        std::function<bool(int)> processMesh;
        std::function<bool(int)> processNode;
        std::function<bool(int)> processSkin;

        processSampler = [&](int tinySamplerIdx)->bool {
            if (tinySamplerIdx < 0) return true;
            if (gltfSamplerToEngine.find(tinySamplerIdx) != gltfSamplerToEngine.end()) return true;
            if (tinySamplerIdx >= static_cast<int>(model.samplers.size())) return false;
            GLTFSampler s;
            if (!getSampler(modelNamePrefix, s, model, model.samplers[tinySamplerIdx])) return false;
            int engIdx = static_cast<int>(out.samplers.size());
            out.samplers.push_back(s);
            gltfSamplerToEngine[tinySamplerIdx] = engIdx;
            return true;
            };

        processTexture = [&](int tinyTexIdx)->bool {
            if (tinyTexIdx < 0) return true;
            if (gltfTexToEngine.find(tinyTexIdx) != gltfTexToEngine.end()) return true;
            if (tinyTexIdx >= static_cast<int>(model.textures.size())) return false;
            const tinygltf::Texture& t = model.textures[tinyTexIdx];
            if (t.source < 0 || t.source >= static_cast<int>(model.images.size())) return false;
            GLTFTexture engTex;
            if (!getTexture(modelNamePrefix, engTex, model, t)) return false;
            int tinySamplerIdx = t.sampler;
            if (tinySamplerIdx >= 0) {
                if (!processSampler(tinySamplerIdx)) return false;
                auto it = gltfSamplerToEngine.find(tinySamplerIdx);
                engTex.smaplerIndex = (it != gltfSamplerToEngine.end()) ? it->second : -1;
            }
            else {
                engTex.smaplerIndex = -1;
            }
            int engIdx = static_cast<int>(out.textures.size());
            out.textures.push_back(engTex);
            gltfTexToEngine[tinyTexIdx] = engIdx;
            return true;
            };

        processMaterial = [&](int tinyMatIdx)->bool {
            if (tinyMatIdx < 0) return true;
            if (gltfMatToEngine.find(tinyMatIdx) != gltfMatToEngine.end()) return true;
            if (tinyMatIdx >= static_cast<int>(model.materials.size())) return false;
            GLTFMaterial engMat;
            if (!getMaterial(modelNamePrefix, engMat, model, model.materials[tinyMatIdx])) return false;
            auto tryRemapTexField = [&](int& field) {
                if (field < 0) return true;
                if (!processTexture(field)) return false;
                auto it = gltfTexToEngine.find(field);
                field = (it != gltfTexToEngine.end()) ? it->second : -1;
                return true;
                };
            if (!tryRemapTexField(engMat.baseColorTexture)) return false;
            if (!tryRemapTexField(engMat.metallicRoughnessTexture)) return false;
            if (!tryRemapTexField(engMat.normalTexture)) return false;
            if (!tryRemapTexField(engMat.occlusionTexture)) return false;
            if (!tryRemapTexField(engMat.emissiveTexture)) return false;

            int engIdx = static_cast<int>(out.materials.size());
            out.materials.push_back(engMat);
            gltfMatToEngine[tinyMatIdx] = engIdx;
            return true;
            };

        processMesh = [&](int tinyMeshIdx)->bool {
            if (tinyMeshIdx < 0) return true;
            if (gltfMeshToEngine.find(tinyMeshIdx) != gltfMeshToEngine.end()) return true;
            if (tinyMeshIdx >= static_cast<int>(model.meshes.size())) return false;
            out.meshes.push_back({});
            GLTFMesh& engMesh = out.meshes.back();
            if (!getMesh(modelNamePrefix, engMesh, model, model.meshes[tinyMeshIdx])) return false;
            for (size_t i = 0; i < engMesh.materialIdx.size(); ++i) {
                int tinyMatIdx = engMesh.materialIdx[i];
                if (tinyMatIdx < 0) {
                    engMesh.materialIdx[i] = -1;
                    continue;
                }
                if (!processMaterial(tinyMatIdx)) return false;
                auto it = gltfMatToEngine.find(tinyMatIdx);
                if (it == gltfMatToEngine.end()) return false;
                engMesh.materialIdx[i] = it->second;
            }
            int engIdx = static_cast<int>(out.meshes.size() - 1);
            gltfMeshToEngine[tinyMeshIdx] = engIdx;
            return true;
            };

        // skin -> skeleton
        processSkin = [&](int tinySkinIdx)->bool {
            if (tinySkinIdx < 0) return true;
            if (gltfSkinToEngine.find(tinySkinIdx) != gltfSkinToEngine.end()) return true;
            if (tinySkinIdx >= static_cast<int>(model.skins.size())) return false;
            out.skeletons.push_back({});
            GLTFSkeleton& sk = out.skeletons.back();
            if (!getSkeleton(modelNamePrefix, sk, model, model.skins[tinySkinIdx])) return false;
            int engIdx = static_cast<int>(out.skeletons.size() - 1);
            gltfSkinToEngine[tinySkinIdx] = engIdx;
            return true;
            };

        processNode = [&](int tinyNodeIdx)->bool {
            if (tinyNodeIdx < 0) return true;
            if (gltfNodeToEngine.find(tinyNodeIdx) != gltfNodeToEngine.end()) return true; // 已处理
            if (tinyNodeIdx >= static_cast<int>(model.nodes.size())) return false;

            out.nodes.push_back({});
            int engNodeIdx = static_cast<int>(out.nodes.size() - 1);
            gltfNodeToEngine[tinyNodeIdx] = engNodeIdx;

            GLTFNode& engNode = out.nodes.back();
            if (!getNode(modelNamePrefix, engNode, model, model.nodes[tinyNodeIdx])) return false;

            for (int childTiny : model.nodes[tinyNodeIdx].children) {
                if (!processNode(childTiny)) return false;
            }

            // 处理此 node 引用的 mesh & skin
            int tinyMeshIdx = model.nodes[tinyNodeIdx].mesh;
            if (tinyMeshIdx >= 0) {
                if (!processMesh(tinyMeshIdx)) return false;
                // remap node.meshIndex 为 engine 索引
                auto it = gltfMeshToEngine.find(tinyMeshIdx);
                engNode.meshIndex = (it != gltfMeshToEngine.end()) ? it->second : -1;
            }
            else engNode.meshIndex = -1;

            int tinySkinIdx = model.nodes[tinyNodeIdx].skin;
            if (tinySkinIdx >= 0) {
                if (!processSkin(tinySkinIdx)) return false;
                auto it = gltfSkinToEngine.find(tinySkinIdx);
                engNode.skinIndex = (it != gltfSkinToEngine.end()) ? it->second : -1;
            }
            else engNode.skinIndex = -1;
            return true;
            };

        // 从每个 scene 的 root node 出发遍历
        for (size_t s = 0; s < model.scenes.size(); ++s) {
            const tinygltf::Scene& sc = model.scenes[s];
            for (int rootTiny : sc.nodes) {
                if (!processNode(rootTiny)) return false;
            }
        }

        for (size_t engIdx = 0; engIdx < out.nodes.size(); ++engIdx) {
            GLTFNode& nn = out.nodes[engIdx];
            std::vector<int> newChildren;
            newChildren.reserve(nn.children.size());
            for (int tinyChild : nn.children) {
                auto it = gltfNodeToEngine.find(tinyChild);
                if (it != gltfNodeToEngine.end()) {
                    newChildren.push_back(it->second);
                }else{
                    assert(false);
                    return false;
                }
            }
            nn.children.swap(newChildren);
        }

        out.scenes.clear();
        for (size_t s = 0; s < model.scenes.size(); ++s) {
            GLTFScene sc;
            sc.name = model.scenes[s].name;
            sc.nodes.clear();
            for (int tinyRoot : model.scenes[s].nodes) {
                auto it = gltfNodeToEngine.find(tinyRoot);
                if (it == gltfNodeToEngine.end()) {
                    return false;
                }
                sc.nodes.push_back(it->second);
            }
            out.scenes.push_back(std::move(sc));
        }
        out.modelName = modelName;
        return true;
    }

	bool GLTFLoaderPrivate::getGLTFScene(GLTFScene& out, const tinygltf::Scene& scene)
	{
        out.name = scene.name;
        out.nodes = scene.nodes;
        return true;
	}

	bool GLTFLoaderPrivate::getSkeleton(const Name& name, GLTFSkeleton& out, const tinygltf::Model& model, const tinygltf::Skin& skin)
    {
        out.root = -1;
        out.joints.clear();

        if (skin.joints.empty()) {
            return false;
        }
        if (skin.inverseBindMatrices < 0) {
            return false;
        }
        const auto& nodes = model.nodes;
        const auto& jointNodes = skin.joints;
        const size_t jointCount = jointNodes.size();
        const tinygltf::Accessor& acc = model.accessors[skin.inverseBindMatrices];
        if (acc.type != TINYGLTF_TYPE_MAT4 || acc.count != jointCount) {
            return false;
        }

        const tinygltf::BufferView& bv = model.bufferViews[acc.bufferView];
        const tinygltf::Buffer& buf = model.buffers[bv.buffer];
        const uint8_t* base =
            buf.data.data() + bv.byteOffset + acc.byteOffset;
        std::unordered_map<int, int> nodeToJoint;
        nodeToJoint.reserve(jointCount);
        for (size_t i = 0; i < jointCount; ++i) {
            nodeToJoint[jointNodes[i]] = static_cast<int>(i);
        }
        out.joints.resize(jointCount);
        for (size_t i = 0; i < jointCount; ++i) {
            const int nodeIdx = jointNodes[i];
            if (nodeIdx < 0 || nodeIdx >= static_cast<int>(nodes.size())) {
                return false;
            }

            GLTFJoint& j = out.joints[i];
            const tinygltf::Node& n = nodes[nodeIdx];

            // ---- bind pose local TRS ----
            if (n.translation.size() == 3) {
                j.translation = vec3(
                    float(n.translation[0]),
                    float(n.translation[1]),
                    float(n.translation[2]));
            }

            if (n.rotation.size() == 4) {
                // glTF: [x, y, z, w]
                j.rotation = vec4(
                    float(n.rotation[0]),
                    float(n.rotation[1]),
                    float(n.rotation[2]),
                    float(n.rotation[3]));
            }

            if (n.scale.size() == 3) {
                j.scale = vec3(
                    float(n.scale[0]),
                    float(n.scale[1]),
                    float(n.scale[2]));
            }

            // ---- inverse bind matrix ----
            const float* m = reinterpret_cast<const float*>(
                base + sizeof(float) * 16 * i);

            mat4 ibm(1.0f);
            for (int r = 0; r < 4; ++r) {
                for (int c = 0; c < 4; ++c) {
                    ibm[c][r] = m[r * 4 + c];
                }
            }
            j.inverseBindMatrix = ibm;
            j.name = n.name;
            j.parent = -1;
            j.children.clear();
        }

        bool oneRoot = false;
        for (size_t i = 0; i < jointCount; ++i) {
            int nodeIdx = jointNodes[i];

            int parentNode = -1;
            for (size_t n = 0; n < nodes.size(); ++n) {
                for (int c : nodes[n].children) {
                    if (c == nodeIdx) {
                        parentNode = static_cast<int>(n);
                        break;
                    }
                }
                if (parentNode != -1) break;
            }

            auto it = nodeToJoint.find(parentNode);
            if (it != nodeToJoint.end()) {
                int parentJoint = it->second;
                out.joints[i].parent = parentJoint;
                out.joints[parentJoint].children.push_back(static_cast<int>(i));
            }
            else {
                
                if (!oneRoot)
                    oneRoot = true;
                else 
                    return false;

            }
        }

        out.root = -1;
        if (skin.skeleton >= 0) {
            auto it = nodeToJoint.find(skin.skeleton);
            if (it != nodeToJoint.end()) {
                out.root = it->second;
            }
        }

        if (out.root == -1) {
            for (size_t i = 0; i < jointCount; ++i) {
                if (out.joints[i].parent == -1) {
                    out.root = static_cast<int>(i);
                    break;
                }
            }
        }
        if (out.root != -1) {
            addSocketForSkeleton(name, out, model, skin, nodeToJoint);
        }
        else {
            return false;
        }
        return true;
    }

    bool GLTFLoaderPrivate::getMesh(const Name& name, GLTFMesh& out, const tinygltf::Model& model, const tinygltf::Mesh& mesh)
    {
        std::vector<StandardModelVertex> vertex;
        std::vector<uint32_t> indice;
        std::vector<Render::SubMesh> submeshes;
        std::vector<int> matIdx;
        bool isSuccess = packPrimitiveToStandardVertices(model, mesh, vertex, indice, submeshes, matIdx);
        if (!isSuccess)return false;
        out.materialIdx = matIdx;
        MeshData meshdata = MeshData(
            (void*)vertex.data(),sizeof(StandardModelVertex) * vertex.size(), vertex.size(), (void*)indice.data(), indice.size(), IndexType::Uint32
        );
        meshdata.addAttribute(Render::VertexFormat::Float3,VertexSemantic::Position,offsetof(StandardModelVertex,position));
        meshdata.addAttribute(Render::VertexFormat::Float3, VertexSemantic::Normal, offsetof(StandardModelVertex, normal));
        meshdata.addAttribute(Render::VertexFormat::Float2, VertexSemantic::TexCoord0, offsetof(StandardModelVertex, uv_0));
        meshdata.addAttribute(Render::VertexFormat::Float2, VertexSemantic::TexCoord1, offsetof(StandardModelVertex, uv_1));
        meshdata.addAttribute(Render::VertexFormat::UByte4N, VertexSemantic::Color0, offsetof(StandardModelVertex, color_u8x4_pack));
        meshdata.setSubMeshes(submeshes);
        Name thisMeshResName = Name(name.str()+ "_" + mesh.name);
        out.mesh = ResourceSystem::instance()->registerResource(ResourceName::Mesh, thisMeshResName, meshdata.toMeshResource());
        out.name = mesh.name;
        return true;
    }

    bool GLTFLoaderPrivate::getMaterial(
        const Name& name,
        GLTFMaterial& out,
        const tinygltf::Model& model,
        const tinygltf::Material& mat)
    {
        // name
        out.name = name.str();
        if (!mat.name.empty()) {
            out.name += "_" + mat.name;
        }

        if (mat.emissiveFactor.size() >= 3) {
            out.emissiveFactor[0] = static_cast<float>(mat.emissiveFactor[0]);
            out.emissiveFactor[1] = static_cast<float>(mat.emissiveFactor[1]);
            out.emissiveFactor[2] = static_cast<float>(mat.emissiveFactor[2]);
        }
        else {
            out.emissiveFactor[0] = out.emissiveFactor[1] = out.emissiveFactor[2] = 0.0f;
        }
        out.emissiveTexture = mat.emissiveTexture.index;
        out.emissiveTextureCoord = mat.emissiveTexture.texCoord;

        if (mat.alphaMode == "MASK") {
            out.alphaMode = GLTFAlphaMode::Mask;
            out.alphaCutoff = static_cast<float>(mat.alphaCutoff);
        }
        else if (mat.alphaMode == "BLEND") {
            out.alphaMode = GLTFAlphaMode::Blend;
        }
        else {
            out.alphaMode = GLTFAlphaMode::Opaque;
        }

        out.doubleSided = mat.doubleSided;

        const auto& pbr = mat.pbrMetallicRoughness;

        if (pbr.baseColorFactor.size() >= 4) {
            out.baseColorFactor[0] = static_cast<float>(pbr.baseColorFactor[0]);
            out.baseColorFactor[1] = static_cast<float>(pbr.baseColorFactor[1]);
            out.baseColorFactor[2] = static_cast<float>(pbr.baseColorFactor[2]);
            out.baseColorFactor[3] = static_cast<float>(pbr.baseColorFactor[3]);
        }
        else {
            out.baseColorFactor[0] = out.baseColorFactor[1] =
                out.baseColorFactor[2] = 1.0f;
            out.baseColorFactor[3] = 1.0f;
        }

        out.baseColorTexture = pbr.baseColorTexture.index;
        out.baseColorTexCoord = pbr.baseColorTexture.texCoord;

        out.metallicFactor = static_cast<float>(pbr.metallicFactor);
        out.roughnessFactor = static_cast<float>(pbr.roughnessFactor);
        out.metallicRoughnessTexture = pbr.metallicRoughnessTexture.index;
        out.metallicRoughnessTexCoord = pbr.metallicRoughnessTexture.texCoord;

        out.normalTexture = mat.normalTexture.index;
        out.normalTexCoord = mat.normalTexture.texCoord;
        out.normalScale = static_cast<float>(mat.normalTexture.scale);

        out.occlusionTexture = mat.occlusionTexture.index;
        out.occlusionTexCoord = mat.occlusionTexture.texCoord;
        out.occlusionStrength = static_cast<float>(mat.occlusionTexture.strength);
        return true;
    }
    bool GLTFLoaderPrivate::getTexture(const Name& name, GLTFTexture& out, const tinygltf::Model& model, const tinygltf::Texture& texture)
    {
        auto imgIdx = texture.source;
        if (imgIdx < 0 || imgIdx >= model.images.size()) {
            return false;
        }
        const tinygltf::Image& image = model.images[imgIdx];
        if (image.pixel_type != TINYGLTF_COMPONENT_TYPE_BYTE && image.pixel_type != TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE) {
            assert(false);
            return false;
        }
        auto imageData = image.image.data();
        auto imageRaw = ImageRaw::createImageRaw(
            image.width, image.height, image.component
        );
        assert(imageRaw->getByteSize() == image.image.size());
        memcpy(imageRaw->getImageRaw(), image.image.data(), image.image.size());
        Name textureName = Name(name.str() + "_" + texture.name);
        out.name = textureName.str();
        out.texture = ResourceSystem::instance()->registerResource(ResourceName::Texture, textureName, imageRaw->toTextureResource());
        out.smaplerIndex = texture.sampler;
        delete imageRaw;
        imageRaw = nullptr;
        return true;
    }
    bool GLTFLoaderPrivate::getSampler(const Name& name, GLTFSampler& out, const tinygltf::Model& model, const tinygltf::Sampler& sampler)
    {
        out.addressS = fromGltfToAddressMode(sampler.wrapS);
        out.addressT = fromGltfToAddressMode(sampler.wrapT);
        out.minFilter = fromGltfToFilter(sampler.minFilter);
        out.magFilter = fromGltfToFilter(sampler.magFilter);
        return true;
    }
    bool GLTFLoaderPrivate::getNode(const Name& name, GLTFNode& out, const tinygltf::Model& model, const tinygltf::Node& node)
    {
        std::string nameOfNode = node.name;

        if (node.translation.size() == 3) {
            out.translation[0] = node.translation[0];
            out.translation[1] = node.translation[1];
            out.translation[2] = node.translation[2];
        }

        if (node.rotation.size() == 0) {
            out.rotation[3] = 1;
        }
        else {
            out.rotation[0] = node.rotation[0];
            out.rotation[1] = node.rotation[1];
            out.rotation[2] = node.rotation[2];
            out.rotation[3] = node.rotation[3];
        }
        
        if (node.scale.size() == 3) {
            out.scale[0] = node.scale[0];
            out.scale[1] = node.scale[1];
            out.scale[2] = node.scale[2];
        }
        else {
			out.scale[0] = 1;
			out.scale[1] = 1;
			out.scale[2] = 1;
        }

        for (auto&& idx : node.children) {
            out.children.push_back(idx);
        }

        out.name = nameOfNode;
        out.skinIndex = node.skin;
        out.meshIndex = node.mesh;
        out.skinIndex = node.skin;
        out.children = node.children;
        return true;
    }
    MaterialPtr GLTFLoaderPrivate::createPBRMaterialFromGLTFMaterial(GLTFModel* model, const GLTFMaterial& gltfMat)
    {
        Name matName = Name(gltfMat.name);
        auto pbrMatPtr = ResourceSystem::instance()->getResource<Material>(ResourceName::Material, matName);
        if (!pbrMatPtr) {
            pbrMatPtr = MaterialManager::instance()->createMaterial<PBRMaterial>(matName,getPBRMaterialTemplate(&gltfMat));
            pbrMatPtr->addMaterialPassToRender(PassName::MainCameraPass);
        }
        else {
            return pbrMatPtr;
        }

        PBRMaterial* pbrMat = dynamic_cast<PBRMaterial*>(pbrMatPtr.get());
        //Setup material
        pbrMat->setBaseColor(vec4(
            gltfMat.baseColorFactor[0],
            gltfMat.baseColorFactor[1],
            gltfMat.baseColorFactor[2],
            gltfMat.baseColorFactor[3]
        ));
        pbrMat->setMetallic(gltfMat.metallicFactor);
        pbrMat->setRoughness(gltfMat.roughnessFactor);
        pbrMat->setAOStrength(gltfMat.occlusionStrength);
        pbrMat->setNormalScale(gltfMat.normalScale);
        pbrMat->setEmissive(vec3(gltfMat.emissiveFactor[0],
            gltfMat.emissiveFactor[1],
            gltfMat.emissiveFactor[2]
        ));
        if (gltfMat.baseColorTexture >= 0) {
            TexturePtr texture = model->textures[gltfMat.baseColorTexture].texture;
            SamplerPtr sampler = nullptr;
            if (texture) {
				auto samplerDesc = fromGltfSamplerToSamplerDesc(model->samplers[model->textures[gltfMat.baseColorTexture].smaplerIndex]);
                sampler = SamplerResourceManager::instance()->getOrCreateSampler(samplerDesc);
            }
            pbrMat->setBaseColorTexture(texture, sampler);
        }
        else {
            pbrMat->setBaseColorTexture(nullptr, nullptr);
        }

        if (gltfMat.baseColorTexture >= 0) {
            const auto& gltfTex = model->textures[gltfMat.baseColorTexture];
            TexturePtr texture = gltfTex.texture;
            SamplerPtr sampler = nullptr;
            if (texture) {
                bool useUV0 = true;
                auto samplerDesc = fromGltfSamplerToSamplerDesc(model->samplers[gltfTex.smaplerIndex]);
                sampler = SamplerResourceManager::instance()->getOrCreateSampler(samplerDesc);
                useUV0 = (gltfMat.baseColorTexCoord == 0);
                pbrMat->setBaseColorTexture(texture, sampler, useUV0);
            }
        }
        else {
            pbrMat->setBaseColorTexture(nullptr, nullptr);
        }

        if (gltfMat.metallicRoughnessTexture >= 0) {
            const auto& gltfTex = model->textures[gltfMat.metallicRoughnessTexture];
            TexturePtr texture = gltfTex.texture;
            SamplerPtr sampler = nullptr;
            if (texture) {
                bool useUV0 = true;
                auto samplerDesc = fromGltfSamplerToSamplerDesc(model->samplers[gltfTex.smaplerIndex]);
                sampler = SamplerResourceManager::instance()->getOrCreateSampler(samplerDesc);
                useUV0 = (gltfMat.metallicRoughnessTexCoord == 0);
                pbrMat->setMetallicRoughnessTexture(texture, sampler, useUV0);
            }
        }
        else {
            pbrMat->setMetallicRoughnessTexture(nullptr, nullptr);
        }

        if (gltfMat.normalTexture >= 0) {
            const auto& gltfTex = model->textures[gltfMat.normalTexture];
            TexturePtr texture = gltfTex.texture;
            SamplerPtr sampler = nullptr;
            if (texture) {
                bool useUV0 = true;
                auto samplerDesc = fromGltfSamplerToSamplerDesc(model->samplers[gltfTex.smaplerIndex]);
                sampler = SamplerResourceManager::instance()->getOrCreateSampler(samplerDesc);
                useUV0 = (gltfMat.normalTexCoord == 0);
                pbrMat->setNormalTexture(texture, sampler, useUV0);
            }
        }
        else {
            pbrMat->setNormalTexture(nullptr, nullptr);
        }

        if (gltfMat.occlusionTexture >= 0) {
            const auto& gltfTex = model->textures[gltfMat.occlusionTexture];
            TexturePtr texture = gltfTex.texture;
            SamplerPtr sampler = nullptr;
            if (texture) {
                bool useUV0 = true;
                auto samplerDesc = fromGltfSamplerToSamplerDesc(model->samplers[gltfTex.smaplerIndex]);
                sampler = SamplerResourceManager::instance()->getOrCreateSampler(samplerDesc);
                useUV0 = (gltfMat.occlusionTexCoord == 0);
                pbrMat->setAOTexture(texture, sampler, useUV0);
            }
        }
        else {
            pbrMat->setAOTexture(nullptr, nullptr);
        }
        return pbrMatPtr;
    }
}