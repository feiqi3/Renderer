#include "animation/Skeleton.h"
#include "common/Singleton.h"
#include "common/ResourceManager.h"

namespace Render {

	class SkeletonResource : public IResource {
	public:
		const Anm::Skeleton* getSkeleton() const;
		static const Name& typeName();
		virtual const Name& getTypeName() const override;
		virtual ResourceMemory getMemory() const override;

		void setSkeleton(Anm::Skeleton&& rhs);
	private:
		Anm::Skeleton mSkeleton;
	};
}