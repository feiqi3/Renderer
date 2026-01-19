#ifndef GLTF_LOADER_H
#define GLTF_LOADER_H
#include "platform/FileSystem/FileSystem.h"
#include "Renderer/Texture.h"
#include "Renderer/Mesh.h"
#include "common/CommonMath.h"
#include "render_resource_def.h"
#include "render_resource_createinfo.h"
namespace Render {

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
        std::string         name;
        vec4                baseColor;
        int                 baseColorTex;
        float               metallicFactor;
        float               roughnessFactor;
        int                 metallicRoughnessTex;
        bool                doubleSided;
        std::string         alphaMode;
        int                 normalTex;
        int                 occlusionTex;
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
