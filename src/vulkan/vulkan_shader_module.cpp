#include "vulkan/vulkan_shader_module.h"
#include "vulkan/vulkan_shader_reflect.h"
#include <map>
namespace Render::Vulkan {
    std::vector<DescritporSetInfo> assembleDescriptorSetInfo(const std::vector<ShaderModuleDescriptorsInfo>& descritpors)
    {
        std::map<int, std::vector<rs_descriptor>> mSets;

        //Merge descriptor set in different shader stages
        for (auto&& set : descritpors) {
            auto& vec = mSets[set.setIdx];
            vec.insert(vec.end(), set.mInfo.begin(), set.mInfo.end());
        }

        for (auto&& [setIdx, set] : mSets) {
            //Condition: one descriptor is visible in different stages.
            std::map<int, rs_descriptor> mDesMap;
            for (auto&& descriptor : set) {
                auto itor = mDesMap.find(descriptor.binding);
                if (itor == mDesMap.end())
                {
                    mDesMap.insert({ descriptor.binding ,descriptor });
                }
                else {
                    auto& exsitDes = itor->second;
                    if (exsitDes.count == descriptor.count &&
                        exsitDes.type == descriptor.type
                        ) {
                        exsitDes.shaderVisibleStage |= descriptor.shaderVisibleStage;
                    }
                    else {
                        assert(0 && "Descriptor Set has different declearation across mutiple shader stages");
                        return {};
                    }
                }
            }

            //Write back real descriptor
            set.clear();
            for (auto&& [bindingIdx, descriptor] : mDesMap) {
                set.push_back(descriptor);
            }
        }

        std::vector< DescritporSetInfo> infos;
        infos.reserve(mSets.size());

        for (auto&& [setid, perShaderSetInfo] : mSets) {
            infos.push_back({});
            auto& info = infos[infos.size() - 1];
            info.setIdx = setid;
            info.layoutHash.mDescriptors = perShaderSetInfo;
            info.layoutHash.init();
        }
        return infos;
    }
    std::vector<DescritporSetInfo> getPipelineShaderInfo(const std::vector<rs_shader_module*>& shaders)
    {
        //Descriptor set is defined cross shaders
        std::vector<std::pair<uint16_t, rs_descriptorset_layout_vk*>> setlayouts;
        std::vector<ShaderModuleDescriptorsInfo> diffShaderInfos;
        for (auto&& s : shaders) {
            auto vks = (rs_shader_module_vk*)s;

            if (vks->reflectInfo.empty()) {
                reflectShader(vks, vks->spirvCode.data(), vks->spirvCode.size());
            }

            auto& perShaderInfo = vks->reflectInfo;
            diffShaderInfos.insert(diffShaderInfos.end(), perShaderInfo.begin(), perShaderInfo.end());
        }
        return assembleDescriptorSetInfo(diffShaderInfos);
    }
}
