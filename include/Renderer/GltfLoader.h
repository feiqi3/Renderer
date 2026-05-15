#ifndef GLTF_LOADER_H
#define GLTF_LOADER_H
#include "platform/FileSystem/FileSystem.h"
#include "Renderer/Texture.h"
#include "Renderer/Mesh.h"
#include "common/CommonMath.h"
#include "Renderer/Light.h"
#include "render_resource_def.h"
#include "render_resource_createinfo.h"
#include "MaterialInstance.h"
namespace Render {
    class Scene;
    class Object;
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

        MaterialPtr materialInstance = nullptr;
    };


    struct GLTFMesh {
        std::string         name;
        MeshPtr             mesh;
        vec3                offsetByCenter;
        std::vector<int>    materialIdx; // For each submesh
    };

    struct GLTFNode {
        std::string         name;
        vec3                translation;
        vec4                rotation;
        vec3                scale;                 
        int                 meshIndex;   
        int                 skinIndex;
        int                 lightIndex;
        std::vector<int>    children;
    };

    struct GLTFScene {
        std::string                 name;
        std::vector<int>            nodes;
    };


    struct GLTFJoint {
        std::string name;
        vec3 translation = vec3(0.);
        vec4 rotation = vec4(0, 0, 0, 1.);
        vec3 scale = vec3(1., 1., 1.);
        mat4 inverseBindMatrix = mat4(1.0f);
        int parent = -1;
        bool isJoint = true;
        std::vector<int>  children; //Index to other node in joint list
    };

    struct GLTFSkeleton {
        int root = -1;
        std::vector<GLTFJoint> joints;
    };

    struct GLTFLight {
        std::string name;
        vec3 color;
        float intensity = 1.0f;
        LightType type = LightType::Point;
        float range = 10000.;
    };

    struct GLTFModel {
        std::string                 modelName;
        std::vector<GLTFScene>      scenes;
        std::vector<GLTFNode>       nodes;
        std::vector<GLTFMesh>       meshes;
        std::vector<GLTFMaterial>   materials;
        std::vector<GLTFTexture>    textures;
        std::vector<GLTFSampler>    samplers;
        std::vector<GLTFSkeleton>   skeletons;
        std::vector<GLTFLight>      lights;
    };


	class GLTFLoaderPrivate;
    struct GLTFLoaderSetting {
		bool offsetByCenter = false; //Whether to offset the mesh by its center. This is useful for gltf models exported from 3D software, which usually have their pivot point at the center of the model, but not necessarily at the bottom.
    };
	class GLTFLoader {
	public:
        GLTFLoader(GLTFLoaderSetting loaderSetting = {});
		~GLTFLoader();
		GLTFModel*      createFromFilePath(const std::string& path);
		class Model*    gltfModelToEngimeModel(GLTFModel* gltfModel);    
        Object*         toEngineSceneNode(Scene* scene,GLTFModel* model);
	private:
		GLTFLoaderPrivate* mDp = nullptr;
	};
}
#endif // !GLTF_LOADER_H
