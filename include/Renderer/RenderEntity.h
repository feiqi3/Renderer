#ifndef RENDER_ENTITY_H
#define RENDER_ENTITY_H
#include "render_resource_def.h"
#include "render_resource_createinfo.h"
#include "common/NoCopyable.h"
#include "common/Name.h"
#include "common/CommonMath.h"
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
		inline virtual void updateUniforms(Pass* pass) {};
		inline const RenderInfo& getRenderInfo()const {
			return mRenderInfo;
		}
		RenderInfo& getRenderInfo() {
			return mRenderInfo;
		}

		//Call this when change render entity.
		void clear();
		void setModelMatrix(const mat4& mat);
		const mat4& getModelMatrix()const;
		vec3 getWorldPos()const;

		//Update data like 'World Matrix' 'Model Matrix' etc
		//which will be (and must be) used across all passes
		virtual void updateEntityCommonData();
		virtual void updateEntityCommonDataImpl(Pass* pass);
		rs_drawdata* getEntityCommonDrawData()const;
	private:
		void destroyPass(Pass* pass);
		mat4 mModelMatrix = mat4(1.f);
		friend class RenderSystem;
		//TODO: Write every thing into descriptor
		std::map<Name, Pass*> mPasses;
		
		u64 mEntityDrawDataUpdatedFrame = 0;
		rs_drawdata* mEntityDrawData = nullptr;
		
		RenderInfo mRenderInfo;
	};
};

#endif