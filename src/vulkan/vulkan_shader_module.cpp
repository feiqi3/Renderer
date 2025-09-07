#include "vulkan/vulkan_shader_module.h"
#include "vulkan/vulkan_shader_reflect.h"
#include <map>
namespace Render::Vulkan {
    std::vector<DescritporSetInfo> assembleDescriptorSetInfo(const std::vector<rs_descriptor>& descriptors)
    {
        std::map<
            int, //setIndex
            std::map<int, rs_descriptor> //bindingPos , descriptor
        > setBindingInfo;
        //Merge descriptors in different shader stages into sets 
        for (auto&& descriptor : descriptors) {
            auto vkPos = toVkBindingPos(descriptor.bindingPos);
            auto setItor =
                setBindingInfo.find(
                    vkPos.setIdx
            );
            if (setItor == setBindingInfo.end()) {
                setItor = setBindingInfo.insert({
                vkPos.setIdx,{} }).first;
            }

            auto& bindingMap = setItor->second;
            auto bindingItor = bindingMap.find(vkPos.bindingIdx);
            if (bindingItor == bindingMap.end()) {
                bindingMap.insert({vkPos.bindingIdx, descriptor});
            }
            else {
                auto& existedDescriptor = bindingItor->second;
                if (
                    existedDescriptor.bindingItemName == descriptor.bindingItemName
                    && existedDescriptor.count == descriptor.count
                    && existedDescriptor.size == descriptor.size
                    && existedDescriptor.type == descriptor.type
                ) {
                    existedDescriptor.shaderVisibleStage |= descriptor.shaderVisibleStage;
                }
            }
        }
        std::vector<DescritporSetInfo> retVal;

        for (auto& [setIdx, setMap] : setBindingInfo) {
            DescritporSetInfo setInfo{ .setIdx = setIdx };
            for (auto&& [bindingIdx, descriptor] : setMap) {
                setInfo.layoutHash.mDescriptors.emplace_back(std::move(descriptor));
            }
            setInfo.layoutHash.init();
            assert(setInfo.layoutHash.checkValid() == true);
            retVal.push_back(setInfo);
        }

        return retVal;
    }
    std::vector<DescritporSetInfo> getPipelineShaderInfo(rs_shader_module_vk** shaders, size_t num)
    {
        //Descriptor set is defined cross shaders
        std::vector<std::pair<uint16_t, rs_descriptorset_layout_vk*>> setlayouts;
        std::vector<rs_descriptor> info;
        for(auto i = 0;i < num;++i){
            rs_shader_module_vk* s = shaders[i];
            auto vks = (rs_shader_module_vk*)s;

            if (vks->reflectInfo.empty()) {
                reflectShader(vks, vks->spirvCode.data(), vks->spirvCode.size());
            }
            auto& perShaderInfo = vks->reflectInfo;
            info.insert(info.end(), perShaderInfo.begin(), perShaderInfo.end());
        }
        return assembleDescriptorSetInfo(info);
    }
}
