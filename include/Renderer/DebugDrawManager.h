#ifndef DEBUG_DRAW_MANAGER_H_
#define DEBUG_DRAW_MANAGER_H_
#include "common/Singleton.h"
#include "common/CommonMath.h"
#include "function/AABB.h"
#include <memory>
namespace Render {

	class DebugDrawManager : public Singleton<DebugDrawManager>{
	public:
		DebugDrawManager();
		~DebugDrawManager();
		void drawPoint(const vec3& pos,const vec4& color);
		void drawAABB(AxisAlignedBoundingBox& aabb, const vec4& color);

		void onRender(Camera* cam);
	private:
		void initDebugDrawInfo();
		void init();
	private:
		std::unique_ptr<class DebugDrawManagerPrivate> mDp = nullptr;
	};

}

#endif