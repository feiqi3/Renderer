#include "Renderer/MaterialInstance.h"
#include "common/ResourceSystem.h"
#include <assert.h>

namespace Render {
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
		mem.cpuMemory += (uint32_t)(mParameterMap.size() * sizeof(std::pair<std::string, _ParameterPair>));
		return mem;
	}

	void Material::OnUnload() {
		mParameterMap.clear();
		mState = ResourceLoadState::Unloaded;
	}

	void Material::OnUpdateParam(Pass* pass)
	{
	}

	std::optional<Material::_ParameterPair*> Render::Material::getParameterInfo(const std::string& paramName)
	{
		auto itor = mParameterMap.find(paramName);
		if (itor == mParameterMap.end()) {
			for (const auto& [name, pass] : m_template->getMaterialMap()) {
				auto bindingInfoOption = pass->getBindingInfoByName(paramName);
				if (bindingInfoOption.has_value()) {
					_ParameterPair bpair{};
					bpair.bindingPos = bindingInfoOption->bindingPos;
					bpair.parameterType = bindingInfoOption->type;
					bpair.parameterImageType = ImageType::Invalid;
					if (bindingInfoOption->type == UniformType::UniformBuffer) {
						//Give a baisc size.
						bpair.var.setUniformBuffer(nullptr, bindingInfoOption->size);
					}
					if (bindingInfoOption->type == UniformType::Texture) {
						bpair.parameterImageType = bindingInfoOption->imageType;
					}
					auto ret = this->mParameterMap.insert({ paramName, std::move(bpair) });
					return &(ret.first)->second;
				}
			}
		}
		else {
			return &itor->second;
		}
		return std::nullopt;
	}

	void Material::bindParameter(const std::string& paramName, TexturePtr tex)
	{
		auto bpair = this->getParameterInfo(paramName);



		if (bpair.has_value()) {

			auto viewType = tex->getRsImage()->defaultView.viewKey.getViewType();
			if (viewType != ImageType::Invalid && viewType != (*bpair)->parameterImageType) {
				//assert(false && "invalid binding");
			}

			auto& pair = *bpair.value();
			if (pair.parameterType == UniformType::Texture ||
				pair.parameterType == UniformType::StorageImage ||
				pair.parameterType == UniformType::InputAttachment) {
				if (tex != nullptr) {
					pair.var.set(tex);
				}
				else {
					pair.var.set(ResourceSystem::instance()->getDefaultResource<Texture>(ResourceName::Texture));
				}
			}
		}
	}

	void Material::bindParameter(const std::string& paramName, rs_buffer* buffer)
	{
		auto bpair = this->getParameterInfo(paramName);
		if (bpair.has_value()) {
			auto& pair = *bpair.value();
			if (pair.parameterType == UniformType::ConstantBuffer ||
				pair.parameterType == UniformType::StorageBuffer) {
				pair.var.set(buffer); 
			}
		}
	}

	void Material::bindParameter(const std::string& paramName, SamplerPtr sampler)
	{
		auto bpair = this->getParameterInfo(paramName);
		if (bpair.has_value()) {
			auto& pair = *bpair.value();
			if (pair.parameterType == UniformType::Sampler) {
				pair.var.set(sampler);
			}
		}
	}

	void Material::bindParameter(const std::string& paramName, const void* data, u32 size)
	{
		auto bpair = this->getParameterInfo(paramName);
		if (bpair.has_value()) {
			auto& pair = *bpair.value();
			if (pair.parameterType == UniformType::UniformBuffer) {
				pair.var.setUniformBuffer(data, size);
			}
		}
	}

	void Material::uploadUniform(Pass* pass)
	{
		this->OnUpdateParam(pass);
		for (auto& [name, bpair] : mParameterMap) {
			if (!bpair.var.hasResource()) {
				continue;
			}
			auto sys = RenderSystem::instance();
			switch (bpair.parameterType) {
			case UniformType::StorageBuffer:
			case UniformType::ConstantBuffer:
				sys->updateUniform(bpair.bindingPos, bpair.var.getRsBuffer(), pass);
				break;

			case UniformType::UniformBuffer:
				assert(false && "Not supported here");
				break;

			case UniformType::StorageImage:
			case UniformType::Texture:
			case UniformType::InputAttachment:
				sys->updateUniform(bpair.bindingPos, bpair.var.getTexture()->getRsImage(), pass);
				break;

			case UniformType::Sampler:
				sys->updateUniform(bpair.bindingPos, bpair.var.getSampler()->getRsSampler(), pass);
				break;

			case UniformType::Count:
			default:
				assert(false && "Invalid UniformType encountered!");
				break;
			}
		}
	}

	MaterialPass* Material::getMaterialPass(const Name& name)
	{
		return m_template->getMaterialPass(name);
	}

	void Material::addMaterialPassToRender(const Name& passName)
	{
		this->passNamesToRender.push_back(passName);
	}
	MaterialPass* Material::getMaterialPassToRender(const Name& passName)
	{
		auto pass = this->m_template->getMaterialPass(passName);
		return pass;
	}
	void Material::setRenderOrder(u32 order)
	{
		mRenderOrder = order;
	}
	u32 Material::getRenderOrder() const
	{
		return mRenderOrder;
	}

}