#ifndef DEBUG_DRAW_MANAGER_H_
#define DEBUG_DRAW_MANAGER_H_

#include "common/Singleton.h"
#include "common/CommonMath.h"
#include "function/AABB.h"
#include "function/Plane.h"
#include <memory>

namespace Render {

	class DebugDrawManager : public Singleton<DebugDrawManager> {
	public:
		DebugDrawManager();
		~DebugDrawManager();

		void drawCube(const vec3& pos, const quat& rotation, const vec4& color, float size = 1.f);
		void drawPoint(const vec3& pos, const vec4& color, float size = 1.f);
		void drawQuad(const vec3& center, const vec2& size, const vec4& color);
		void drawPlane(const Plane& plane, const vec2& size, const vec4& color);
		void drawPlane(const Plane& plane, const vec3& center, const vec2& size, const vec4& color);
		void drawAABB(const AxisAlignedBoundingBox& aabb, const vec4& color);
		void drawLine(const vec3& beg,const vec3& end,const vec4& color, float width );
		void init();
		void onRender(class Camera* cam);

	private:
		void initDebugDrawInfo();
	private:
		std::unique_ptr<class DebugDrawManagerPrivate> mDp = nullptr;
	};

}

#endif