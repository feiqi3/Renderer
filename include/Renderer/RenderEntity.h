#ifndef RENDER_ENTITY_H
#define RENDER_ENTITY_H
#include "common/NoCopyable.h"
#include "MaterialVarient.h"
#include <map>
namespace Render {
	struct RenderPendingData {
		ResourceType type;
		union {
			void* dataPtr;		//Data of Per frame update
			void* resourcePtr;  //Ptr of Resource, Like Texture/Storage buffer
		};
		uint32_t size = 0;
	};
	class Pass;
	class RenderEntity : Common::NonCopyable{
	public:
		virtual bool isRenderReady() { return true; }
		virtual ~RenderEntity() ;

		virtual void update() {}

		virtual MaterialTemplate* getMaterial();
		void setMaterialTemplate(MaterialTemplate* material);
		rs_buffer* getIndexBuffer();
		IndexType getIndexType()const;
		void setIndexBuffer(rs_buffer* buffer,IndexType type);
		void init();
		std::vector<VertexBindingInfo >& getBindingBuffers();
		rs_drawdata* getDrawData(const Name& passName);
		Pass* createPass(const Name& passName);
		void destroyPass(const Name& passName);
		Pass* getPass(const Name& passName);
		const RenderInfo& getRenderInfo()const {
			return mRenderInfo;
		}
		RenderInfo& getRenderInfo() {
			return mRenderInfo;
		}

	private:

		void destroyPass(Pass* pass);

		friend class RenderSystem;
		//TODO: Write every thing into descriptor
		std::map<Name, Pass*> mPasses;
		MaterialTemplate* mMaterial = 0;
		RenderInfo mRenderInfo;
	};
};

#endif