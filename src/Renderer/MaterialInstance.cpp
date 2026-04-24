#include "Renderer/MaterialInstance.h"
#include "common/ResourceSystem.h"
#include <assert.h>
#include <cstring> // for std::memcpy

namespace Render {

	// =========================================================================
	// =========================================================================
	namespace EngineBindlessAPI {
		uint32_t GetGlobalSamplerIndex(rs_sampler* sampler) { return 0; }
		void UpdateGlobalSamplerArray(rs_sampler* sampler, uint32_t index) {}
		uint32_t GetGlobalTextureIndex(rs_image* img) { return 0; }
		void UpdateGlobalTextureArray(rs_image* img, uint32_t index) {}
		uint64_t GetBufferDeviceAddress(rs_buffer* buffer) { return 0ULL; }
		uint32_t GetBlockBufferArrayIndex(rs_buffer* buffer) { return 0ul; }
		//Datasize: 4 or 8 --> array offset or DBA
		uint64_t GetBlockBufferBindingData(rs_buffer* buffer, uint32_t& datasize) {
			datasize = 4;
			return 0;
		}
	}
	// =========================================================================

	const Name& Material::typeName() {
		static const Name name("Material");
		return name;
	}

	Material::Material(const MaterialTemplatePtr& matTplt)
	{
		assert(matTplt.get() != nullptr);
		m_template = matTplt;
		mState = ResourceLoadState::Loaded;
	}

	Material::~Material()
	{
		OnUnload();
	}

	const Name& Material::getTypeName() const {
		return typeName();
	}

	ResourceMemory Material::getMemory() const {
		ResourceMemory mem{ 0, 0 };
		mem.cpuMemory = (uint32_t)sizeof(*this);
		mem.cpuMemory += (uint32_t)(mBindingSlots.size() * sizeof(_ParameterPair));
		mem.cpuMemory += (uint32_t)(mBindlessItems.size() * sizeof(_BindlessItem));
		for (const auto& pp : mBindingSlots) {
			if (pp.location.type == ResourceLocationType::BindingSlot &&
				pp.location.descriptorInfo.type == UniformType::UniformBuffer &&
				!(pp.varArr.size() == 0)) {
				void* data = nullptr; uint32_t size = 0;
				pp.varArr[0].getData(&data, &size);
				mem.cpuMemory += size;
			}
		}
		return mem;
	}

	void Material::OnUnload() {
		mName2BindingSlot.clear();
		mName2BindlessSlot.clear();
		mBindingPos2BindingSlot.clear();
		mBindingSlots.clear();
		mBindlessItems.clear();
		mState = ResourceLoadState::Unloaded;
	}

	void Material::OnUpdateParam(Pass* pass) {}

	void Material::ensureParameterRegistered(const Name& paramName)
	{
		if (mName2BindingSlot.count(paramName) || mName2BindlessSlot.count(paramName)) {
			return;
		}

		for (const auto& [name, pass] : m_template->getMaterialMap()) {
			auto bindingInfoOption = pass->getBindingInfoByName(paramName);
			if (bindingInfoOption.has_value()) {
				const auto& loc = *bindingInfoOption;

				if (loc.type == ResourceLocationType::BindlessSlot) {
					_BindlessItem bindlessItem{};
					bindlessItem.location = loc;
					bindlessItem.keepAliveRefs.resize(loc.bindlessInfo.count);
					mBindlessItems.push_back(bindlessItem);
					mName2BindlessSlot[paramName] = static_cast<uint32_t>(mBindlessItems.size() - 1);

					//Find father UBO
					auto fatherUBOIt = mBindingPos2BindingSlot.find(loc.bindingPos);
					if (fatherUBOIt == mBindingPos2BindingSlot.end()) {
						const auto& passResources = pass->getResourceLocations();
						for (const auto& res : passResources) {
							if (res.type == ResourceLocationType::BindingSlot &&
								res.bindingPos == loc.bindingPos) {
								ensureParameterRegistered(res.itemName);
								break;
							}
						}
					}

				}
				else if (loc.type == ResourceLocationType::BindingSlot) {
					_ParameterPair pp;
					pp.location = loc;
					pp.parameterImageType = ImageType::Invalid;

					pp.varArr.resize(loc.descriptorInfo.count);
					if (loc.descriptorInfo.type == UniformType::UniformBuffer) {
						pp.varArr[0].setUniformBuffer(nullptr, loc.descriptorInfo.size);
					}

					mBindingSlots.push_back(std::move(pp));
					uint32_t slotIdx = static_cast<uint32_t>(mBindingSlots.size() - 1);
					mName2BindingSlot[paramName] = slotIdx;
					mBindingPos2BindingSlot[loc.bindingPos] = slotIdx;
				}
				return;
			}
		}
	}

	void Material::bindParameter(const std::string& paramName, TexturePtr tex, int element)
	{
		Name name(paramName);
		ensureParameterRegistered(name);
		if (RenderSystem::instance()->isBindlessEnabled()) {

			if (auto it = mName2BindlessSlot.find(name); it != mName2BindlessSlot.end()) {
				auto& bindlessItem = mBindlessItems[it->second];
				const auto& loc = bindlessItem.location;
				if (element >= loc.bindlessInfo.count) {
					assert(false && "Bindless Texture array out of bounds!");
					return;
				}

				uint32_t globalIndex = 0;
				if (tex != nullptr) {
					globalIndex = EngineBindlessAPI::GetGlobalTextureIndex(tex->getRsImage());
					EngineBindlessAPI::UpdateGlobalTextureArray(tex->getRsImage(), globalIndex);
					bindlessItem.keepAliveRefs[element].set(tex);
				}
				else {
					globalIndex = 0;
					bindlessItem.keepAliveRefs[element].set(TexturePtr(nullptr));
				}

				uint32_t stride = loc.bindlessInfo.stride > 0 ? loc.bindlessInfo.stride : sizeof(uint32_t);
				uint32_t writeOffset = loc.bindlessInfo.offset + (element * stride);

				auto uboIt = mBindingPos2BindingSlot.find(loc.bindingPos);
				if (uboIt != mBindingPos2BindingSlot.end()) {
					auto& parentUBO = mBindingSlots[uboIt->second];
					void* data = nullptr; uint32_t size = 0;
					parentUBO.varArr[0].getData(&data, &size);

					if (data && writeOffset + sizeof(uint32_t) <= size) {
						std::memcpy(static_cast<uint8_t*>(data) + writeOffset, &globalIndex, sizeof(uint32_t));
					}
				}
				return;
			}
		}

		if (auto it = mName2BindingSlot.find(name); it != mName2BindingSlot.end()) {
			auto& pp = mBindingSlots[it->second];
			if (element < pp.varArr.size()) {
				if (tex != nullptr) pp.varArr[element].set(tex);
				else pp.varArr[element].set(ResourceSystem::instance()->getDefaultResource<Texture>(ResourceName::Texture));
			}
		}
	}

	void Material::bindParameter(const std::string& paramName, rs_buffer* buffer, int element)
	{
		//FIX ME: need buffer offset
		Name name(paramName);
		ensureParameterRegistered(name);
		if (RenderSystem::instance()->isBindlessEnabled()) {
			if (auto it = mName2BindlessSlot.find(name); it != mName2BindlessSlot.end()) {
				auto& bindlessItem = mBindlessItems[it->second];
				const auto& loc = bindlessItem.location;
				if (element >= loc.bindlessInfo.count) {
					assert(false && "Bindless DBA array out of bounds!");
					return;
				}
				auto uboIt = mBindingPos2BindingSlot.find(loc.bindingPos);
				void* tarData = nullptr;
				u32 size = 0;
				if (uboIt != mBindingPos2BindingSlot.end()) {
					auto& parentUBO = mBindingSlots[uboIt->second];
					parentUBO.varArr[0].getData(&tarData, &size);
				}
				uint32_t stride = loc.bindlessInfo.stride;
				uint32_t writeOffset = loc.bindlessInfo.offset + (element * stride);
				uint32_t bindingDataSize = 0;
				uint64_t bindingData = EngineBindlessAPI::GetBlockBufferBindingData(buffer, bindingDataSize);
				assert(bindingDataSize == 4 || bindingDataSize == 8);
				assert(writeOffset + bindingDataSize <= size);

				assert(tarData != nullptr && "ERROR");
				if (buffer != nullptr) {
					bindlessItem.keepAliveRefs[element].set(buffer);
					if (bindingDataSize == 8) {
						//DBA 
						uint64_t bufferIdx = bindingData;
						std::memcpy(static_cast<uint8_t*>(tarData) + writeOffset, &bufferIdx, bindingDataSize);
					}
					else {
						//Buffer Array
						uint32_t bufferIdx = bindingData;
						std::memcpy(static_cast<uint8_t*>(tarData) + writeOffset, &bufferIdx, bindingDataSize);
					}
				}
				else {
					bindlessItem.keepAliveRefs[element].set((rs_buffer*)nullptr);
					if (bindingDataSize == 8) {
						//DBA 
						uint64_t bufferIdx = 0ull;
						std::memcpy(static_cast<uint8_t*>(tarData) + writeOffset, &bufferIdx, sizeof(uint64_t));
					}
					else {
						//Buffer Array
						uint32_t bufferIdx = 0ul;
						std::memcpy(static_cast<uint8_t*>(tarData) + writeOffset, &bufferIdx, sizeof(uint32_t));
					}

				}
				return;
			}
		}

		if (auto it = mName2BindingSlot.find(name); it != mName2BindingSlot.end()) {
			auto& pp = mBindingSlots[it->second];
			if (element < pp.varArr.size()) {
				auto pType = pp.location.descriptorInfo.type;
				if (pType == UniformType::ConstantBuffer || pType == UniformType::StorageBuffer) {
					pp.varArr[element].set(buffer);
				}
			}
		}
	}

	void Material::bindParameter(const std::string& paramName, SamplerPtr sampler, int element)
	{
		Name name(paramName);
		ensureParameterRegistered(name);

		if (RenderSystem::instance()->isBindlessEnabled()) {
			if (auto it = mName2BindlessSlot.find(name); it != mName2BindlessSlot.end()) {
				auto& bindlessItem = mBindlessItems[it->second];
				const auto& loc = bindlessItem.location;

				if (element >= loc.bindlessInfo.count) {
					assert(false && "Bindless Sampler array out of bounds!");
					return;
				}

				auto uboIt = mBindingPos2BindingSlot.find(loc.bindingPos);
				void* tarData = nullptr;
				u32 size = 0;
				if (uboIt != mBindingPos2BindingSlot.end()) {
					auto& parentUBO = mBindingSlots[uboIt->second];
					parentUBO.varArr[0].getData(&tarData, &size);
				}

				uint32_t stride = loc.bindlessInfo.stride > 0 ? loc.bindlessInfo.stride : sizeof(uint32_t);
				uint32_t writeOffset = loc.bindlessInfo.offset + (element * stride);

				assert(writeOffset + sizeof(uint32_t) <= size);
				assert(tarData != nullptr && "ERROR: Father UBO not found");

				if (sampler != nullptr) {
					bindlessItem.keepAliveRefs[element].set(sampler);

					uint32_t samplerIdx = EngineBindlessAPI::GetGlobalSamplerIndex(sampler->getRsSampler());
					EngineBindlessAPI::UpdateGlobalSamplerArray(sampler->getRsSampler(), samplerIdx);

					std::memcpy(static_cast<uint8_t*>(tarData) + writeOffset, &samplerIdx, sizeof(uint32_t));
				}
				else {
					bindlessItem.keepAliveRefs[element].set(SamplerPtr(nullptr));

					uint32_t nullIdx = 0ul;
					std::memcpy(static_cast<uint8_t*>(tarData) + writeOffset, &nullIdx, sizeof(uint32_t));
				}
				return;
			}
		}
		if (auto it = mName2BindingSlot.find(name); it != mName2BindingSlot.end()) {
			auto& pp = mBindingSlots[it->second];
			if (element < pp.varArr.size()) {
				if (pp.location.type == ResourceLocationType::BindingSlot &&
					pp.location.descriptorInfo.type == UniformType::Sampler) {
					pp.varArr[element].set(sampler);
				}
			}
		}
	}

	void Material::bindParameter(const std::string& paramName, const void* in_data, u32 in_size)
	{
		Name name(paramName);
		ensureParameterRegistered(name);

		if (auto it = mName2BindingSlot.find(name); it != mName2BindingSlot.end()) {
			auto& pp = mBindingSlots[it->second];
			if (pp.location.type == ResourceLocationType::BindingSlot &&
				pp.location.descriptorInfo.type == UniformType::UniformBuffer) {
				pp.varArr[0].setUniformBuffer(in_data, in_size);
			}
		}
	}

	void Material::uploadUniform(Pass* pass)
	{
		this->OnUpdateParam(pass);
		auto sys = RenderSystem::instance();

		// ==========================================
		// ==========================================

		for (auto& pp : mBindingSlots) {
			auto& vars = pp.varArr;
			for (int i = 0; i < vars.size(); ++i) {
				auto& var = vars[i];
				if (!var.hasResource()) continue;

				switch (pp.location.descriptorInfo.type) {
				case UniformType::StorageBuffer:
				case UniformType::ConstantBuffer:
					sys->updateUniform(pp.location.bindingPos, i, var.getRsBuffer(), 0, 0, pass);
					break;
				case UniformType::UniformBuffer:
				{
					void* dataptr = nullptr; uint32_t size = 0;
					var.getData(&dataptr, &size);
					sys->updateUniformBufferData(pp.location.bindingPos, dataptr, size, pass);
					break;
				}
				case UniformType::StorageImage:
				case UniformType::Texture:
				case UniformType::InputAttachment:
				{
					if (var.isTexture()) sys->updateUniform(pp.location.bindingPos, i, var.getTexture()->getRsImage(), pass);
					else if (var.isTextureView()) sys->updateUniform(pp.location.bindingPos, i, var.getTextureView()->view, pass);
					break;
				}
				case UniformType::Sampler:
					sys->updateUniform(pp.location.bindingPos, i, var.getSampler()->getRsSampler(), pass);
					break;
				default:
					assert(false && "Invalid UniformType encountered!");
					break;
				}
			}
		}
	}

	MaterialPass* Material::getMaterialPass(const Name& name) { return m_template->getMaterialPass(name); }
	void Material::addMaterialPassToRender(const Name& passName) { this->passNamesToRender.push_back(passName); }
	MaterialPass* Material::getMaterialPassToRender(const Name& passName) { return this->m_template->getMaterialPass(passName); }
	void Material::setRenderOrder(u32 order) { mRenderOrder = order; }
	u32 Material::getRenderOrder() const { return mRenderOrder; }

} // namespace Render