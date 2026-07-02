#ifndef VIRTUAL_MATERIAL_H_
#define VIRTUAL_MATERIAL_H_
#include "Renderer/MaterialInstance.h"
#include "common/Name.h"
#include "render_resource.h"
namespace Render {
	struct ShaderCommonData {
		rs_drawdata* sceneDrawData = nullptr;
		rs_drawdata* cameraDrawData = nullptr;
	};
	class VirtualMaterial : public Material{
	public:
		inline ShaderCommonData* getPassDrawdata(const Name& passName) {
			auto itor = mCommonDrawdata.find(passName);
			if (itor == mCommonDrawdata.end()) {
				return nullptr;
			}
			return &itor->second;
		}
	private:
		std::map<Name, ShaderCommonData> mCommonDrawdata;
	};
}

#endif //VIRTUAL_MATERIAL_H_