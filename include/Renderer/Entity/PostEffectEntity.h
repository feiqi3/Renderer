#include "Renderer/RenderEntity.h"
namespace Render{
	//In shader: read into U_ImageFrom
	//In shader: read into U_ImageTo   

	class PostEffectEntity :public RenderEntity{
	public: 
		PostEffectEntity();
		void setBlitImageFrom(rs_binding_pos imageFromPos,rs_image* image);
		void setBlitImageTo(rs_binding_pos imageFromPos, rs_image* image);
	};

	class BlitEntity : public PostEffectEntity {
	public:

		
	};
}