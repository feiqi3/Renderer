#ifndef RENDER_DEBUGER_H
#define RENDER_DEBUGER_H
#include "common/NoCopyable.h"
#include <string>
namespace Render {
	struct rs_commandbuffer;
	class RenderMarker : public Common::NonCopyable{
	public:
		RenderMarker(rs_commandbuffer* cmd, const char* label,float r,float g,float b,float a);
		~RenderMarker();
	private:
		rs_commandbuffer* mCommandBuffer;
	
	public:
		void insertMarker(rs_commandbuffer* cmd, const char* label, float r, float g, float b, float a);
	};
}

#endif