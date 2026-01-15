#include "render_resource_createinfo.h"
namespace Render {
	 ImageViewKey::ImageViewKey() : value(0) {
		bits.layerCount		= 1;
		bits.mipCount		= 1;
		bits.aspect			= (uint64_t)ViewAspect::Color;
		bits.viewType		= (uint64_t)ImageType::V2D;
	}

	 ImageViewKey::ImageViewKey(uint64_t rawValue) : value(rawValue) {}

	 ImageViewKey& ImageViewKey::setAspect(ViewAspect aspect) {
		bits.aspect = static_cast<uint64_t>(aspect);
		return *this;
	}

	 ImageViewKey& ImageViewKey::setViewType(ImageType type) {
		bits.viewType = static_cast<uint64_t>(type);
		return *this;
	}

	 ImageViewKey& ImageViewKey::setBaseMip(uint32_t mip) {
		bits.baseMip = mip; 
		return *this;
	}

	 ImageViewKey& ImageViewKey::setMipCount(uint32_t count) {
		bits.mipCount = count;
		return *this;
	}

	 ImageViewKey& ImageViewKey::setBaseLayer(uint32_t layer) {
		bits.baseLayer = layer;
		return *this;
	}

	 ImageViewKey& ImageViewKey::setLayerCount(uint32_t count) {
		bits.layerCount = count;
		return *this;
	}

	 ViewAspect ImageViewKey::getAspect() const {
		return static_cast<ViewAspect>(bits.aspect);
	}

	 ImageType ImageViewKey::getViewType() const {
		return static_cast<ImageType>(bits.viewType);
	}

	 uint32_t ImageViewKey::getBaseMip() const {
		return static_cast<uint32_t>(bits.baseMip);
	}

	 uint32_t ImageViewKey::getMipCount() const {
		return static_cast<uint32_t>(bits.mipCount);
	}

	 uint32_t ImageViewKey::getBaseLayer() const {
		return static_cast<uint32_t>(bits.baseLayer);
	}

	 uint32_t ImageViewKey::getLayerCount() const {
		return static_cast<uint32_t>(bits.layerCount);
	}

	 bool ImageViewKey::operator==(const ImageViewKey& other) const {
		return value == other.value;
	}

	 bool ImageViewKey::operator!=(const ImageViewKey& other) const {
		return value != other.value;
	}

	 bool ImageViewKey::isValid() const {
		return value != 0;
	}
}