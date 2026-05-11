#include "Renderer/RenderEntity.h"
namespace Render{
	//In shader: read into U_ImageFrom
	//In shader: read into U_ImageTo   

	class PostEffectEntity :public RenderEntity{
	public: 
		PostEffectEntity(const std::string& effectName, const std::string& vsName, const std::string& psName);
	};
}