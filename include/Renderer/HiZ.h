#ifndef HIZ_H_
#define HIZ_H_
#include "common/CoreDefs.h"
#include "Renderer/ComputeKernel.h"
#include "Renderer/Texture.h"
namespace Render {
	class HiZ {
	public:

		void setDepth(const TexturePtr& tex);
		TexturePtr getHiZPyramid();
	
		void execute(rs_commandbuffer* cmdBuffer);
		HiZ();
		~HiZ();
	private:
		void setupResource();
	private:
		TexturePtr mDepth;
		TexturePtr mHiZPyramid;
		ComputeKernel* mHizKernel;
		Name mParamLastZ;
		Name mParamWriteZ;
	};
}

#endif !HIZ_H_