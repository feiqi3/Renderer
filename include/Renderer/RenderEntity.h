#ifndef RENDER_ENTITY_H
#define RENDER_ENTITY_H
#include "render_resource_def.h"
#include "render_resource_createinfo.h"
#include "common/NoCopyable.h"
#include "common/Name.h"
#include <map>
namespace Render {
	struct RenderPendingData {
		UniformType type;
		union {
			void* dataPtr;		//Data of Per frame update
			void* resourcePtr;  //Ptr of Resource, Like Texture/Storage buffer
		};
		uint32_t size = 0;
	};
	class Pass;
	class MaterialPass;
	class Material;
	struct rs_drawdata;
	struct rs_buffer;
	struct rs_commandbuffer;
	class RenderEntity : Common::NonCopyable{
	public:
		virtual bool isRenderReady() { return true; }
		virtual ~RenderEntity() ;

		virtual Material* getMaterial() = 0;
		rs_buffer* getIndexBuffer();
		IndexType getIndexType()const;
		void setIndexBuffer(rs_buffer* buffer,IndexType type);
		std::vector<VertexBindingInfo >& getBindingBuffers();
		rs_drawdata* getDrawData(const Name& passName);
		Pass* createPass(const Name& passName);
		void destroyPass(const Name& passName);
		Pass* getPass(const Name& passName);
		virtual float distToCam()const { return 0.f; }
		virtual void updateUniforms(rs_commandbuffer* cmd, MaterialPass* pass) = 0;
		inline const RenderInfo& getRenderInfo()const {
			return mRenderInfo;
		}
		RenderInfo& getRenderInfo() {
			return mRenderInfo;
		}

		//Call this when change render entity.
		void clear();
	private:

		void destroyPass(Pass* pass);

		friend class RenderSystem;
		//TODO: Write every thing into descriptor
		std::map<Name, Pass*> mPasses;
		RenderInfo mRenderInfo;
	};
};

#endif