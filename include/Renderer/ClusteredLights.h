#ifndef CLUSTERED_LIGHTS_H_
#define CLUSTERED_LIGHTS_H_
#include "GPUShared/SceneLightsCullData.h"
#include "Renderer/Texture.h"
#include "Renderer/RenderSystem.h"
#include <memory>

namespace Render {

	class Camera;
	class Scene;
	class HizClusteredLightPrivate;

	class HizClusteredLight {
	public:
		HizClusteredLight();
		~HizClusteredLight();

		HizClusteredLight(const HizClusteredLight&) = delete;
		HizClusteredLight& operator=(const HizClusteredLight&) = delete;

		void setHiZTexture(const TexturePtr& hizTex);

		void setLightListBuffer(rs_buffer* lightBuffer, uint32_t lightCount);

		void draw(rs_commandbuffer* cmdBuffer, Camera* cam, Scene* scene);

		rs_buffer* getFroxelLightDataBuffer() const;

		const GPUShared::ClusterInfo& getClusterInfo()const;
	private:
		TexturePtr mHizTex;
		rs_buffer* mExternalLightBuffer = nullptr;
		uint32_t   mExternalLightCount = 0;
		GPUShared::ClusterInfo mClusterInfo;
		std::unique_ptr<HizClusteredLightPrivate> mDp;
	};

} // namespace Render

#endif // CLUSTERED_LIGHTS_H_