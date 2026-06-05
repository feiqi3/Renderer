#include "Renderer/StandardPRBRenderEntity.h"
namespace Render {
	AxisAlignedBoundingBox Render::StandardPBRRenderEntity::getWorldBounding()
	{
		return this->mAABB.transform(this->getModelMatrix());
	}
}
