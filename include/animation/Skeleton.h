#if !defined(SKELETON_H_)
#define SKELETON_H_

#include "common/CommonMath.h"
#include "common/Name.h"
#include <vector>
namespace Render::Anm {

	struct Joint {
		quat		rotation;
		vec3		translation;
		vec3		scale;
		int32_t		parent;
	};

	class Skeleton {
		std::vector<Name>  mJointsName;
		std::vector<Joint> mJoints;
		std::vector<mat4>  mJointsLocalTRS;
		std::vector<mat4>  mInverseBindingMats;
	};



}
#endif//!SKELETON_H_