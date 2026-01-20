#ifndef GLTF_LOADER_H
#define GLTF_LOADER_H
#include "platform/FileSystem/FileSystem.h"
#include "Renderer/Texture.h"
#include "Renderer/Mesh.h"
#include "common/CommonMath.h"
#include "render_resource_def.h"
#include "render_resource_createinfo.h"
namespace Render {

    enum class GLTFAlphaMode : uint8_t {
        Opaque,
        Mask,           //AlphaTest
        Blend
    };

    struct GLTFSampler {
        Filter          minFilter = Filter::Linear;
        Filter          magFilter = Filter::Linear;
        AddressMode     addressS = AddressMode::Repeat;
        AddressMode     addressT = AddressMode::Repeat;
    };

    struct GLTFTexture {
        std::string         name;
        TexturePtr          texture;
        int                 smaplerIndex;
    };

    struct GLTFMaterial {
        std::string name;

        // Alpha
        GLTFAlphaMode alphaMode = GLTFAlphaMode::Opaque;
        float alphaCutoff = 0.5f;
        bool doubleSided = false;

        // Base color / PBR
        float baseColorFactor[4] = { 1.f, 1.f, 1.f, 1.f };
        int baseColorTexture = -1;
        int baseColorTexCoord = 0;

        float metallicFactor = 1.0f;
        float roughnessFactor = 1.0f;
        int metallicRoughnessTexture = -1;
        int metallicRoughnessTexCoord = 0;

        // Normal
        int normalTexture = -1;
        int normalTexCoord = 0;
        float normalScale = 1.0f;

        // Occlusion --> usually share the same texture with m-r
        int occlusionTexture = -1;
        int occlusionTexCoord = 0;
        float occlusionStrength = 1.0f;

        // Emissive
        float emissiveFactor[3] = { 0.f, 0.f, 0.f };
        int emissiveTexture = -1;
        int emissiveTextureCoord = 0;
    };


    struct GLTFMesh {
        std::string         name;
        MeshPtr             mesh;
    };

    struct GLTFNode {
        std::string         name;
        vec3                translation;
        vec4                rotation;
        vec3                scale;                 
        int                 meshIndex;      
        std::vector<int>    children;
    };

    struct GLTFScene {
        std::vector<GLTFNode>       nodes;
        std::vector<int>            rootNodes;
        std::vector<GLTFMesh>       meshes;
        std::vector<GLTFMaterial>   materials;
        std::vector<GLTFTexture>    textures;
        std::vector<GLTFSampler>    samplers;
    };

    struct GLTFJoint {
        //this is not a joint list ---> may contain not joint node
        //so you need extra bake to get right trs
        int nodeIndex = -1;                
        int parent = -1;                  
        std::vector<int> children;        
        mat4 inverseBindMatrix = mat4(1.0f); 
    };

	class GLTFLoaderPrivate;
	class GLTFLoader {
	public:
		GLTFLoader();
		~GLTFLoader();
		GLTFScene* createFromFilePath(const std::string& path);
	private:
		GLTFLoaderPrivate* mDp = nullptr;
	};
}
#endif // !GLTF_LOADER_H
