#include "vulkan/vulkan_shader_module.h"
#include "vulkan/vulkan_shader_reflect.h"
#include "render_log.h"
#include <map>
namespace Render::Vulkan {
    bool assembleBindlessInfo(const std::vector<BindlessInfo>& bindlessInfoList, std::vector<BindlessInfo>& out)
    {

        std::map<rs_binding_pos,
            std::map<uint32_t, Name>
        > bindlessInfoMap;
        for (const auto& bindlessInfo : bindlessInfoList) {
            auto vkPos = toVkBindingPos(bindlessInfo.bindingPos);
            auto bindlessItor = bindlessInfoMap.find(bindlessInfo.bindingPos);
            bool hasCreated = true;
            if (bindlessItor == bindlessInfoMap.end()) {
                //Assemble here
                out.push_back(bindlessInfo);
                bindlessItor = bindlessInfoMap.insert({ bindlessInfo.bindingPos,{} }).first;
                hasCreated = false;
            }
            auto& slotsMap = bindlessItor->second;
            for (int i = 0; i < bindlessInfo.slots.size();++i) {
                auto& slot = bindlessInfo.slots[i];
                auto slotItor = slotsMap.find(slot.offset);
                bool mismatch = false;
                if (slotItor == slotsMap.end()) {
                    if (hasCreated) {
                        Log::error("Bindless resource size mismatch at Set " + std::to_string(vkPos.setIdx) +
                            " Binding " + std::to_string(vkPos.bindingIdx));
                        //Size mismatch
                        mismatch = true;
                    }
                    else { 
                        slotsMap.insert({ slot.offset,slot.bindlessItemName });
                        continue;
                    }
                }
                
                if (slotItor->second != slot.bindlessItemName) {
                    Log::error("Bindless resource conflict at Set " + std::to_string(vkPos.setIdx) +
                        " Binding " + std::to_string(vkPos.bindingIdx) + " Offset: " + std::to_string(slot.offset));
                   //Binding name mismatch
                    mismatch = true;
                }

                if (mismatch) {
                    return false;
                }
                
            }
        }

        return true;
    }
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
				else {
					Log::error("Shader resource conflict at Set " + std::to_string(vkPos.setIdx) +
						" Binding :" + std::to_string(vkPos.bindingIdx));
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
    PipelineLayoutInfo getPipelineShaderInfo(rs_shader_module_vk** shaders, size_t num)
    {
        //Descriptor set is defined cross shaders
        std::vector<rs_descriptor> info;
        std::vector<BindlessInfo> bindlessInfoList;
        std::vector<BindlessInfo> outBindlessInfo;
        PipelineLayoutInfo ret{};
        for(auto i = 0;i < num;++i){
            rs_shader_module_vk* s = shaders[i];
            auto vks = (rs_shader_module_vk*)s;

            auto& perShaderInfo = vks->rflInfo.bindingInfo;
            info.insert(info.end(), perShaderInfo.begin(), perShaderInfo.end());
            bindlessInfoList.insert(bindlessInfoList.end(), vks->rflInfo.bindlessInfo.begin(), vks->rflInfo.bindlessInfo.end());
        }
        if (!assembleBindlessInfo(bindlessInfoList, outBindlessInfo)) {
            Log::error("Bindless info mismatch.");
            assert(false);
        }

        auto descriptorSetList = assembleDescriptorSetInfo(info);
        ret.bindlessInfo = std::move(outBindlessInfo);
        ret.setInfo = std::move(descriptorSetList);
        return ret;
    }
}
