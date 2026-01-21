#include "Renderer/GltfLoader.h"
#include "tiny_gltf.h"
#include "platform/FileSystem/FileSystem.h"
#include "common/ResourceSystem.h"
#include <iostream>
#include "Renderer/ModelVertex.h"
#include "Renderer/Mesh.h"

namespace {

    enum class TypeOfVertexElement {
        Vec4,
        Vec3,
        Vec2,
        Uint8X4,
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

    static float ReadComponentAsFloat(const uint8_t* src, int componentType, bool normalized, int compIndex) {
        // compIndex is index of component within element (for types with multi-components laid contiguous)
        const uint8_t* ptr = src;
        switch (componentType) {
        case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE: {
            uint8_t v = reinterpret_cast<const uint8_t*>(ptr)[compIndex];
            if (normalized) return static_cast<float>(v) / 255.0f;
            return static_cast<float>(v);
        }
        case TINYGLTF_COMPONENT_TYPE_BYTE: {
            int8_t v = reinterpret_cast<const int8_t*>(ptr)[compIndex];
            if (normalized) {
                // SNORM8: -128 maps to -1, but typical practice divide by 127 and clamp
                float f = static_cast<float>(v) / 127.0f;
                if (f < -1.0f) f = -1.0f;
                return f;
            }
            return static_cast<float>(v);
        }
        case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT: {
            const uint16_t* p = reinterpret_cast<const uint16_t*>(ptr);
            uint16_t v = p[compIndex];
            if (normalized) return static_cast<float>(v) / 65535.0f;
            return static_cast<float>(v);
        }
        case TINYGLTF_COMPONENT_TYPE_SHORT: {
            const int16_t* p = reinterpret_cast<const int16_t*>(ptr);
            int16_t v = p[compIndex];
            if (normalized) {
                float f = static_cast<float>(v) / 32767.0f;
                if (f < -1.0f) f = -1.0f;
                return f;
            }
            return static_cast<float>(v);
        }
        case TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT: {
            const uint32_t* p = reinterpret_cast<const uint32_t*>(ptr);
            uint32_t v = p[compIndex];
            if (normalized) {
                // rarely used for attributes, but implement
                return static_cast<float>(v) / 4294967295.0f;
            }
            return static_cast<float>(v);
        }
        case TINYGLTF_COMPONENT_TYPE_INT: {
            const int32_t* p = reinterpret_cast<const int32_t*>(ptr);
            int32_t v = p[compIndex];
            if (normalized) {
                float f = static_cast<float>(v) / 2147483647.0f;
                if (f < -1.0f) f = -1.0f;
                return f;
            }
            return static_cast<float>(v);
        }
        case TINYGLTF_COMPONENT_TYPE_FLOAT: {
            const float* p = reinterpret_cast<const float*>(ptr);
            return p[compIndex];
        }
        default:
            return 0.0f;
        }
    }

    static uint8_t ConvertComponentToUint8(const uint8_t* src, int componentType, bool normalized, int compIndex) {
        // Convert a single source component into an 0..255 uint8 value.
        // If source is unsigned byte and not normalized, return directly.
        switch (componentType) {
        case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE: {
            return reinterpret_cast<const uint8_t*>(src)[compIndex];
        }
        case TINYGLTF_COMPONENT_TYPE_BYTE: {
            int8_t v = reinterpret_cast<const int8_t*>(src)[compIndex];
            if (normalized) {
                float f = std::max(-1.0f, std::min(1.0f, static_cast<float>(v) / 127.0f));
                return static_cast<uint8_t>(std::round((f * 0.5f + 0.5f) * 255.0f)); // map [-1,1] -> [0,255]
            }
            else {
                int x = static_cast<int>(v);
                if (x < 0) x = 0;
                if (x > 255) x = 255;
                return static_cast<uint8_t>(x);
            }
        }
        case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT: {
            const uint16_t* p = reinterpret_cast<const uint16_t*>(src);
            uint16_t v = p[compIndex];
            if (normalized) {
                float f = static_cast<float>(v) / 65535.0f;
                return static_cast<uint8_t>(std::round(f * 255.0f));
            }
            else {
                // scale-down/clamp
                int x = (v > 255) ? 255 : static_cast<int>(v);
                return static_cast<uint8_t>(x);
            }
        }
        case TINYGLTF_COMPONENT_TYPE_SHORT: {
            const int16_t* p = reinterpret_cast<const int16_t*>(src);
            int16_t v = p[compIndex];
            if (normalized) {
                float f = std::max(-1.0f, std::min(1.0f, static_cast<float>(v) / 32767.0f));
                return static_cast<uint8_t>(std::round((f * 0.5f + 0.5f) * 255.0f));
            }
            else {
                int x = v;
                if (x < 0) x = 0;
                if (x > 255) x = 255;
                return static_cast<uint8_t>(x);
            }
        }
        case TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT: {
            const uint32_t* p = reinterpret_cast<const uint32_t*>(src);
            uint32_t v = p[compIndex];
            if (normalized) {
                float f = static_cast<float>(v) / 4294967295.0f;
                return static_cast<uint8_t>(std::round(f * 255.0f));
            }
            else {
                uint32_t x = (v > 255u) ? 255u : v;
                return static_cast<uint8_t>(x);
            }
        }
        case TINYGLTF_COMPONENT_TYPE_INT: {
            const int32_t* p = reinterpret_cast<const int32_t*>(src);
            int32_t v = p[compIndex];
            if (normalized) {
                float f = std::max(-1.0f, std::min(1.0f, static_cast<float>(v) / 2147483647.0f));
                return static_cast<uint8_t>(std::round((f * 0.5f + 0.5f) * 255.0f));
            }
            else {
                int x = v;
                if (x < 0) x = 0;
                if (x > 255) x = 255;
                return static_cast<uint8_t>(x);
            }
        }
        case TINYGLTF_COMPONENT_TYPE_FLOAT: {
            const float* p = reinterpret_cast<const float*>(src);
            float v = p[compIndex];
            // assume floats representing color/normalized in [0,1], but clamp
            float f = std::max(0.0f, std::min(1.0f, v));
            return static_cast<uint8_t>(std::round(f * 255.0f));
        }
        default:
            return 0;
        }
    }

    bool packToVertex(
        uint8_t* data,
        int offsetInData,          // 当前 attribute 在 Vertex 内的 offset (bytes)
        int offset,                // vertex 起始偏移（base vertex index）
        TypeOfVertexElement type,  // 目标格式
        int stride,                // Vertex stride (bytes)
        const tinygltf::Accessor& acc,
        const tinygltf::BufferView& view,
        const tinygltf::Buffer& buffer
    ) {
        if (acc.count == 0) return false;

        const int srcNumComp = NumComponentsInType(acc.type);
        const int compByteSize = ComponentByteSize(acc.componentType);
        if (compByteSize == 0) return; // unknown component type

        const uint8_t* basePtr = buffer.data.empty() ? nullptr : buffer.data.data();
        if (!basePtr) return false;

        // calculate source base offset
        size_t viewByteOffset = static_cast<size_t>(view.byteOffset);
        size_t accessorByteOffset = static_cast<size_t>(acc.byteOffset);
        const uint8_t* srcElement0 = basePtr + viewByteOffset + accessorByteOffset;

        // source stride: either bufferView.byteStride or tightly packed
        size_t srcStride = view.byteStride ? static_cast<size_t>(view.byteStride)
            : static_cast<size_t>(compByteSize * srcNumComp);

        // iterate through accessor elements, write into destination buffer
        for (size_t i = 0; i < static_cast<size_t>(acc.count); ++i) {
            const uint8_t* src = srcElement0 + i * srcStride;
            uint8_t* dstVertexStart = data + (offset + static_cast<int>(i)) * stride;
            uint8_t* dst = dstVertexStart + offsetInData;

            if (type == TypeOfVertexElement::Uint8X4) {
                // write 4 bytes into dst (no float conversion as primary goal)
                // If accessor has fewer than 4 components, fill remaining with 0 (or 255 for alpha if you prefer)
                for (int c = 0; c < 4; ++c) {
                    if (c < srcNumComp) {
                        uint8_t out = ConvertComponentToUint8(src, acc.componentType, acc.normalized, c);
                        dst[c] = out;
                    }
                    else {
                        // pad: for colors it's common to pad alpha as 255, for others 0.
                        // choose 255 for 4th component if present, else 0.
                        dst[c] = (c == 3) ? 255u : 0u;
                    }
                }
            }
            else {
                // target is float vector (Vec2/3/4)
                float comps[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
                // read available components into comps[]
                for (int c = 0; c < srcNumComp && c < 4; ++c) {
                    comps[c] = ReadComponentAsFloat(src, acc.componentType, acc.normalized, c);
                }
                // If target is Vec4 and source has 3 components, set w=1.0f (common for positions/normals)
                if (type == TypeOfVertexElement::Vec4) {
                    if (srcNumComp == 3) comps[3] = 1.0f;
                    else if (srcNumComp == 1) {
                        // if single scalar, place into x and set others appropriately (y,z=0, w=1)
                        comps[1] = comps[2] = 0.0f;
                        comps[3] = 1.0f;
                    }
                }
                if (type == TypeOfVertexElement::Vec3) {
                    // if source provided 4th component but target is Vec3, ignore w
                }
                if (type == TypeOfVertexElement::Vec2) {
                    // only first two components kept
                }

                // write floats to dst (assume dst is aligned enough; use memcpy to be safe)
                if (type == TypeOfVertexElement::Vec4) {
                    memcpy(dst, comps, sizeof(float) * 4);
                }
                else if (type == TypeOfVertexElement::Vec3) {
                    memcpy(dst, comps, sizeof(float) * 3);
                }
                else if (type == TypeOfVertexElement::Vec2) {
                    memcpy(dst, comps, sizeof(float) * 2);
                }
            }
        }
        return true;
    }

    
    bool packPrimitiveToStandardVertices(
        const tinygltf::Model& model,
        const tinygltf::Mesh& mesh,
        std::vector<Render::StandardModelVertex>& outVertices,
        std::vector<uint32_t>& outIndice,
        std::vector<Render::SubMesh>& outSubmeshes,
        std::vector<int>& outMaterialIndex
    )
    {
        size_t totalCntVtx = 0;
        size_t totalIndice = 0;
        for (auto& prim : mesh.primitives)
        {
            auto itPos = prim.attributes.find("POSITION");
            if (itPos == prim.attributes.end()) {
                return false;
            }
            const tinygltf::Accessor& posAcc = model.accessors[itPos->second];
            size_t vertexCount = static_cast<size_t>(posAcc.count);
            if (vertexCount == 0) return false;
            totalCntVtx += vertexCount;
            const tinygltf::Accessor& indiceAcc = model.accessors[prim.indices];
            totalIndice += indiceAcc.count;
        }
        Render::StandardModelVertex vtx{};
        vtx.color_u8x4_pack = 0xFFFFFFFFu;
        outVertices.resize(totalCntVtx, vtx);
        outIndice.resize(totalIndice, 0);
        size_t baseVertexOffset = 0;
        size_t baseIndiceOffset = 0;
        uint8_t* dstBase = reinterpret_cast<uint8_t*>(outVertices.data());
        int primIdx = 0;
        for (auto& prim : mesh.primitives)
        {
            auto itPos = prim.attributes.find("POSITION");
            const tinygltf::Accessor& posAcc = model.accessors[itPos->second];
            size_t vertexCount = static_cast<size_t>(posAcc.count);
            if (vertexCount == 0) return false;

            int stride = static_cast<int>(sizeof(Render::StandardModelVertex));

            auto tryPackAttribute = [&](const char* attrName, TypeOfVertexElement targetType, size_t offsetInStruct) {
                auto it = prim.attributes.find(attrName);
                if (it == prim.attributes.end()) return;
                int accIdx = it->second;
                if (accIdx < 0 || accIdx >= static_cast<int>(model.accessors.size())) return;
                const tinygltf::Accessor& acc = model.accessors[accIdx];
                if (acc.bufferView < 0 || acc.bufferView >= static_cast<int>(model.bufferViews.size())) return;
                const tinygltf::BufferView& view = model.bufferViews[acc.bufferView];
                if (view.buffer < 0 || view.buffer >= static_cast<int>(model.buffers.size())) return;
                const tinygltf::Buffer& buffer = model.buffers[view.buffer];

                packToVertex(
                    dstBase,
                    static_cast<int>(offsetInStruct),
                    baseVertexOffset,
                    targetType,
                    stride,
                    acc, view, buffer);
                };

            // POSITION -> vec3
            tryPackAttribute("POSITION", TypeOfVertexElement::Vec3, offsetof(Render::StandardModelVertex, position));

            // NORMAL -> vec3
            tryPackAttribute("NORMAL", TypeOfVertexElement::Vec3, offsetof(Render::StandardModelVertex, normal));

            // TEXCOORD_0 -> vec2
            tryPackAttribute("TEXCOORD_0", TypeOfVertexElement::Vec2, offsetof(Render::StandardModelVertex, uv_0));

            // TEXCOORD_1 -> vec2
            tryPackAttribute("TEXCOORD_1", TypeOfVertexElement::Vec2, offsetof(Render::StandardModelVertex, uv_1));

            // COLOR_0 -> uint8x4 packed into uint32_t
            tryPackAttribute("COLOR_0", TypeOfVertexElement::Uint8X4, offsetof(Render::StandardModelVertex, color_u8x4_pack));

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
            sub.vertexOffset = static_cast<int32_t>(baseVertexOffset);
            sub.indexOffset = static_cast<uint32_t>(baseIndiceOffset);
            sub.indexCount = static_cast<uint32_t>(indiceAcc.count);
            outSubmeshes.push_back(sub);
            outMaterialIndex.push_back(prim.material);
            baseIndiceOffset += indiceAcc.count;
            baseVertexOffset += vertexCount;
            primIdx ++ ;
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
        bool            getSkeleton(const Name& name, GLTFSkeleton& out, const tinygltf::Model& model, const tinygltf::Skin& skin);
        bool            getMesh(const Name& name, GLTFMesh& out, const tinygltf::Model& model, const tinygltf::Mesh& mesh);
        bool            getMaterial(const Name& name, GLTFMaterial& out, const tinygltf::Model& model, const tinygltf::Material& mat);
        bool            getTexture(const Name& name, GLTFTexture& out, const tinygltf::Model& model, const tinygltf::Texture& texture);
        bool            getSampler(const Name& name, GLTFSampler& out, const tinygltf::Model& model, const tinygltf::Sampler& sampler);
        bool            getNode(const Name& name, GLTFNode& out, const tinygltf::Model& model, const tinygltf::Node& node);
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

    GLTFScene* Render::GLTFLoader::createFromFilePath(const std::string& path)
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
            return false;
        }
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
    }

    bool GLTFLoaderPrivate::getMesh(const Name& name, GLTFMesh& out, const tinygltf::Model& model, const tinygltf::Mesh& mesh)
    {
        std::vector<StandardModelVertex> vertex;
        std::vector<uint32_t> indice;
        std::vector<Render::SubMesh> submeshes;
        std::vector<int> matIdx;
        bool isSuccess = packPrimitiveToStandardVertices(model, mesh, vertex, indice, submeshes, matIdx);
        if (!isSuccess)return false;
        
        MeshData meshdata = MeshData(
            (void*)vertex.data(),sizeof(StandardModelVertex) * vertex.size(), vertex.size(), (void*)indice.data(), indice.size(), IndexType::Uint32
        );
        meshdata.addAttribute(Render::VertexFormat::Float3,VertexSemantic::Position,offsetof(StandardModelVertex,position));
        meshdata.addAttribute(Render::VertexFormat::Float3, VertexSemantic::Normal, offsetof(StandardModelVertex, position));
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
        if (imgIdx < 0 && imgIdx >= model.images.size()) {
            return false;
        }
        const tinygltf::Image& image = model.images[imgIdx];
        if (image.pixel_type != TINYGLTF_COMPONENT_TYPE_BYTE) {
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
        std::string nameOfNode = name.str() + "_" + node.name;
        out.name = nameOfNode;

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

        out.meshIndex = node.mesh;
        for (auto&& idx : node.children) {
            out.children.push_back(idx);
        }
        return true;
    }
}