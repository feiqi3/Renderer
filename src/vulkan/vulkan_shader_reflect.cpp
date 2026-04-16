#include "render_log.h"
#include "render_function.h"
#include "common/Name.h"
#include "vulkan/vulkan_shader_reflect.h"
#include "vulkan/vulkan_render_function.h"
#include "../3rd/spirv-reflect/spirv_reflect.h"
#include "shaderc/shaderc.hpp"
#include <fstream>
#include <filesystem>
namespace {

    Render::UniformType SpvDescriptorTypeToResourceType(SpvReflectDescriptorType type) {
        using UniformType = Render::UniformType;
        switch (type) {
        case SPV_REFLECT_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
        case SPV_REFLECT_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC:
            return UniformType::UniformBuffer;

        case SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_BUFFER:
        case SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC:
            return UniformType::StorageBuffer;

        case SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_IMAGE:
            return UniformType::StorageImage;

        case SPV_REFLECT_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER:
            assert(0 && "Combined Image Sampler not supported here.");
		case SPV_REFLECT_DESCRIPTOR_TYPE_SAMPLED_IMAGE:
		case SPV_REFLECT_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER:
            return UniformType::Texture;

        case SPV_REFLECT_DESCRIPTOR_TYPE_SAMPLER:
            return UniformType::Sampler;

        case SPV_REFLECT_DESCRIPTOR_TYPE_INPUT_ATTACHMENT:
            return UniformType::InputAttachment;

        //case SPV_REFLECT_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR:
        //    return UniformType::AccelerationStructure;

        case SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER:
            return UniformType::StorageBuffer;

        default:
            // 如果你愿意处理未知类型
            return UniformType::Count; // 或者抛异常 / 打日志
        }
    }

    constexpr VkFormat SpvReflectFormatToVkFormat(SpvReflectFormat fmt) {
        switch (fmt) {
        case SPV_REFLECT_FORMAT_UNDEFINED:           return VK_FORMAT_UNDEFINED;

        case SPV_REFLECT_FORMAT_R16_UINT:            return VK_FORMAT_R16_UINT;
        case SPV_REFLECT_FORMAT_R16_SINT:            return VK_FORMAT_R16_SINT;
        case SPV_REFLECT_FORMAT_R16_SFLOAT:          return VK_FORMAT_R16_SFLOAT;

        case SPV_REFLECT_FORMAT_R16G16_UINT:         return VK_FORMAT_R16G16_UINT;
        case SPV_REFLECT_FORMAT_R16G16_SINT:         return VK_FORMAT_R16G16_SINT;
        case SPV_REFLECT_FORMAT_R16G16_SFLOAT:       return VK_FORMAT_R16G16_SFLOAT;

        case SPV_REFLECT_FORMAT_R16G16B16_UINT:      return VK_FORMAT_R16G16B16_UINT;
        case SPV_REFLECT_FORMAT_R16G16B16_SINT:      return VK_FORMAT_R16G16B16_SINT;
        case SPV_REFLECT_FORMAT_R16G16B16_SFLOAT:    return VK_FORMAT_R16G16B16_SFLOAT;

        case SPV_REFLECT_FORMAT_R16G16B16A16_UINT:   return VK_FORMAT_R16G16B16A16_UINT;
        case SPV_REFLECT_FORMAT_R16G16B16A16_SINT:   return VK_FORMAT_R16G16B16A16_SINT;
        case SPV_REFLECT_FORMAT_R16G16B16A16_SFLOAT: return VK_FORMAT_R16G16B16A16_SFLOAT;

        case SPV_REFLECT_FORMAT_R32_UINT:            return VK_FORMAT_R32_UINT;
        case SPV_REFLECT_FORMAT_R32_SINT:            return VK_FORMAT_R32_SINT;
        case SPV_REFLECT_FORMAT_R32_SFLOAT:          return VK_FORMAT_R32_SFLOAT;

        case SPV_REFLECT_FORMAT_R32G32_UINT:         return VK_FORMAT_R32G32_UINT;
        case SPV_REFLECT_FORMAT_R32G32_SINT:         return VK_FORMAT_R32G32_SINT;
        case SPV_REFLECT_FORMAT_R32G32_SFLOAT:       return VK_FORMAT_R32G32_SFLOAT;

        case SPV_REFLECT_FORMAT_R32G32B32_UINT:      return VK_FORMAT_R32G32B32_UINT;
        case SPV_REFLECT_FORMAT_R32G32B32_SINT:      return VK_FORMAT_R32G32B32_SINT;
        case SPV_REFLECT_FORMAT_R32G32B32_SFLOAT:    return VK_FORMAT_R32G32B32_SFLOAT;

        case SPV_REFLECT_FORMAT_R32G32B32A32_UINT:   return VK_FORMAT_R32G32B32A32_UINT;
        case SPV_REFLECT_FORMAT_R32G32B32A32_SINT:   return VK_FORMAT_R32G32B32A32_SINT;
        case SPV_REFLECT_FORMAT_R32G32B32A32_SFLOAT: return VK_FORMAT_R32G32B32A32_SFLOAT;

        case SPV_REFLECT_FORMAT_R64_UINT:            return VK_FORMAT_R64_UINT;
        case SPV_REFLECT_FORMAT_R64_SINT:            return VK_FORMAT_R64_SINT;
        case SPV_REFLECT_FORMAT_R64_SFLOAT:          return VK_FORMAT_R64_SFLOAT;

        case SPV_REFLECT_FORMAT_R64G64_UINT:         return VK_FORMAT_R64G64_UINT;
        case SPV_REFLECT_FORMAT_R64G64_SINT:         return VK_FORMAT_R64G64_SINT;
        case SPV_REFLECT_FORMAT_R64G64_SFLOAT:       return VK_FORMAT_R64G64_SFLOAT;

        case SPV_REFLECT_FORMAT_R64G64B64_UINT:      return VK_FORMAT_R64G64B64_UINT;
        case SPV_REFLECT_FORMAT_R64G64B64_SINT:      return VK_FORMAT_R64G64B64_SINT;
        case SPV_REFLECT_FORMAT_R64G64B64_SFLOAT:    return VK_FORMAT_R64G64B64_SFLOAT;

        case SPV_REFLECT_FORMAT_R64G64B64A64_UINT:   return VK_FORMAT_R64G64B64A64_UINT;
        case SPV_REFLECT_FORMAT_R64G64B64A64_SINT:   return VK_FORMAT_R64G64B64A64_SINT;
        case SPV_REFLECT_FORMAT_R64G64B64A64_SFLOAT: return VK_FORMAT_R64G64B64A64_SFLOAT;

        default: return VK_FORMAT_UNDEFINED;
        }
    }

    using ShaderIncludeRes = ::Render::ShaderIncludeRes;
    using ShaderIncFindFunc = ::Render::ShaderIncFindFunc;

    ShaderIncludeRes DefaultFindFunc(const std::vector<std::string>& dirs, const std::string& requested_source);

    class FunctionalIncluder : public shaderc::CompileOptions::IncluderInterface {
    public:
        // 搜索目录列表 + 用户回调，返回 ShaderIncludeRes

        FunctionalIncluder(std::vector<std::string> search_dirs,
            ShaderIncFindFunc find_fn)
            : search_dirs_(std::move(search_dirs)),
            find_fn_(std::move(find_fn)) {
            if (!find_fn_) {
                find_fn = DefaultFindFunc;
            }
        }

        ~FunctionalIncluder() override = default;

        shaderc_include_result* GetInclude(const char* requested_source,
            shaderc_include_type /*type*/,
            const char* /*requesting_source*/,
            size_t /*include_depth*/) override {
            // 调用用户回调，传入搜索路径

            ShaderIncludeRes res = find_fn_(search_dirs_, requested_source);
            if (res.FindResult != true)return nullptr;
            // 分配并填充结果
            auto* result = new shaderc_include_result;

            // 复制 source_name
            size_t name_len = res.ShaderName.size();
            char* name_buf = static_cast<char*>(std::malloc(name_len + 1));
            std::memcpy(name_buf, res.ShaderName.data(), name_len);
            name_buf[name_len] = '\0';
            result->source_name = name_buf;
            result->source_name_length = name_len;

            // 复制 content
            size_t content_len = res.ShaderContent.size();
            char* content_buf = static_cast<char*>(std::malloc(content_len + 1));
            std::memcpy(content_buf, res.ShaderContent.data(), content_len);
            content_buf[content_len] = '\0';
            result->content = content_buf;
            result->content_length = content_len;

            result->user_data = nullptr;
            return result;
        }

        void ReleaseInclude(shaderc_include_result* data) override {
            if (!data) return;
            std::free(const_cast<char*>(data->source_name));
            std::free(const_cast<char*>(data->content));
            delete data;
        }

    private:
        std::vector<std::string> search_dirs_;
        ShaderIncFindFunc find_fn_;
    };

    shaderc_shader_kind MapShaderStageToShaderc(::Render::ShaderStage stage) {
        using S = ::Render::ShaderStage;
        switch (stage) {
        case S::Vertex:           return shaderc_vertex_shader;
        case S::Fragment:         return shaderc_fragment_shader;
        case S::Compute:          return shaderc_compute_shader;
        case S::Geometry:         return shaderc_geometry_shader;
        case S::TessControl:      return shaderc_tess_control_shader;
        case S::TessEvaluation:   return shaderc_tess_evaluation_shader;

        case S::RayGen:           return shaderc_raygen_shader;
        case S::AnyHit:           return shaderc_anyhit_shader;
        case S::ClosestHit:       return shaderc_closesthit_shader;
        case S::Miss:             return shaderc_miss_shader;
        case S::Intersection:     return shaderc_intersection_shader;
        case S::Callable:         return shaderc_callable_shader;

        case S::Task:             return shaderc_task_shader;
        case S::Mesh:             return shaderc_mesh_shader;

        default: break;
        }
        return shaderc_glsl_infer_from_source; // 或者抛出异常/assert
    }

    ShaderIncludeRes DefaultFindFunc(const std::vector<std::string>& dirs, const std::string& requested_source) {
        for (const auto& dir : dirs) {
            std::filesystem::path path = std::filesystem::path(dir) / requested_source;
            if (std::filesystem::exists(path)) {
                std::ifstream in(path, std::ios::binary);
                std::string content((std::istreambuf_iterator<char>(in)),
                    std::istreambuf_iterator<char>());
                return ShaderIncludeRes{true, path.string(), content };
            }
        }
        return ShaderIncludeRes{false, "", "Error: include not found: " + requested_source };
    }
}

namespace Render::Vulkan {
	rs_shader_module_vk* compileShader(rs_context_vk* ctx, const ShaderCompileDesc& desc)
	{
		shaderc::CompileOptions options;
		shaderc::Compiler compiler;
        auto includer = std::unique_ptr<shaderc::CompileOptions::IncluderInterface>(new FunctionalIncluder(desc.shaderIncludeDirectories, desc.shaderIncludeFindFunc));
        options.SetIncluder(std::move(includer));
        shaderc_optimization_level lvl = desc.enableOptimize ? shaderc_optimization_level_performance : shaderc_optimization_level_zero;

        options.SetOptimizationLevel(lvl);

        if (desc.generateDebugInfo) {
            options.SetGenerateDebugInfo();
        }

        shaderc_env_version version;
        switch (SHADER_COMPILE_VULKAN_VERSION)
        {
        case 0 : 
            version = shaderc_env_version_vulkan_1_0;
            break;
        case 1:
            version = shaderc_env_version_vulkan_1_1;
            break;
        case 2:
            version = shaderc_env_version_vulkan_1_2;
            break;
        case 3:
            version = shaderc_env_version_vulkan_1_3;
            break;
        default:
            version = shaderc_env_version_vulkan_1_2;
            break;
        }


        options.SetTargetEnvironment(shaderc_target_env_vulkan, version);

        shaderc_source_language lang;

        switch (desc.langType)
        {
        case ShaderLang::HLSL:
            lang = shaderc_source_language_hlsl;
            break;
        case ShaderLang::GLSL:
            lang = shaderc_source_language_glsl;
            break;
        default:
            lang = shaderc_source_language_hlsl;
            break;
        }

        options.SetSourceLanguage(lang);
        //---------------------------------------------//

        //Bug: If any '\0' exist, shader compile will fail;
        int remove_char_cnt = 0;
        for (auto inv_it = desc.shaderSrcCode.crbegin(); inv_it != desc.shaderSrcCode.crend(); ++inv_it) {
            if (*inv_it != '\0') {
                break;
            }
            else {
                ++remove_char_cnt;
            }
        }

        std::string newCode = desc.shaderSrcCode;
        newCode.resize(newCode.size() - remove_char_cnt);
        shaderc::SpvCompilationResult module =
            compiler.CompileGlslToSpv(newCode, MapShaderStageToShaderc(desc.stage),desc.shaderName.c_str(),options);

        if (module.GetCompilationStatus() != shaderc_compilation_status_success) {
            Render::Log::error("Shader Compile Error: " + desc.shaderName + " \nError Reason: " + module.GetErrorMessage());
            return nullptr;
        }

        if (module.GetNumWarnings() > 0) {
            Render::Log::warn("Shader Compile Success with warning: " + desc.shaderName + " \nWarning : " + module.GetErrorMessage());
        }
        VkShaderModule vkModule;
        VkShaderModuleCreateInfo ci{VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO};

        ci.codeSize = sizeof(uint32_t) * (module.end() - module.begin() );
        ci.pCode = module.begin();
        VK_CHECK(vkCreateShaderModule(ctx->device, &ci, 0, &vkModule), { return nullptr; });
        rs_shader_module_vk* shaderModule = new rs_shader_module_vk;
        shaderModule->spirvCode.resize(ci.codeSize);
        std::memcpy(&shaderModule->spirvCode[0], ci.pCode, ci.codeSize);
        shaderModule->native = vkModule;
        shaderModule->shaderName = desc.shaderName;
        shaderModule->shaderStage = desc.stage;
        shaderModule->shaderCode = desc.shaderSrcCode;
        return shaderModule;
	}
    void reflectShader(rs_shader_module_vk* shader, uint32_t* spirv_code, uint64_t codeSize)
    {
        SpvReflectShaderModule shaderModule;
        SpvReflectResult result = spvReflectCreateShaderModule(
            codeSize, spirv_code, &shaderModule);
        if (result != SPV_REFLECT_RESULT_SUCCESS) {
            Log::error("SpirV Reflect wrong" + shader->shaderName);
            return;
        }
        uint32_t input_count = 0;
        std::vector<InputAttribute> attributes;
        std::vector<BindingInfo> bindings;


        spvReflectEnumerateInputVariables(&shaderModule, &input_count, nullptr);
        std::vector<SpvReflectInterfaceVariable*> inputs(input_count);
        spvReflectEnumerateInputVariables(&shaderModule, &input_count, inputs.data());

        for (const auto* var : inputs) {
            if (var->decoration_flags & SPV_REFLECT_DECORATION_BUILT_IN) continue;
            InputAttribute attr{};
            attr.location = var->location;
            attr.format = fromImaegFormatToVertexFormat(ToImageFormat( SpvReflectFormatToVkFormat(var->format)));
            attributes.push_back(attr);
        }

        uint32_t binding_count = 0;
        spvReflectEnumerateDescriptorBindings(&shaderModule, &binding_count, nullptr);
        std::vector<SpvReflectDescriptorBinding*> rflbindings(binding_count);
        spvReflectEnumerateDescriptorBindings(&shaderModule, &binding_count, rflbindings.data());

        for (const auto* binding : rflbindings) {
            BindingInfo bindingInfo{};


            {
                vk_binding_pos pos{ .setIdx = int16_t(binding->set),.bindingIdx = int16_t(binding->binding) };
                auto rsBindingPos = toRsBindingPos(pos);
                bindingInfo.bindingPos = rsBindingPos;
            }

            bindingInfo.bindingItemName = Name(binding->name);
            bindingInfo.count = binding->count;
            bindingInfo.type = SpvDescriptorTypeToResourceType(binding->descriptor_type);

            if (bindingInfo.type == UniformType::UniformBuffer && bindingInfo.bindingItemName.str().starts_with("CBUFFER_")) {
                bindingInfo.type = UniformType::ConstantBuffer;
            }

            if (bindingInfo.type == UniformType::Texture) {
                switch (binding->image.dim)
                {
                case SpvDim1D:
                    bindingInfo.imageType = ImageType::V1D;
                    break;
                case SpvDim2D:
                {
                    if (binding->array.dims_count > 1) {
                        bindingInfo.imageType = ImageType::V2D_Array;
                    }
                    else {
						bindingInfo.imageType = ImageType::V2D;
                    }
					break;
				}
                case SpvDim3D:
                {
					bindingInfo.imageType = ImageType::V3D;
                    break;
                }
				case SpvDimCube:
				{
					if (binding->array.dims_count > 1) {
						bindingInfo.imageType = ImageType::VCube_Array;
					}
					else {
						bindingInfo.imageType = ImageType::VCube;
					}
					break;
				}
                default:
					bindingInfo.imageType = ImageType::V2D;
					break;
                }
            }

            if (binding->resource_type == SPV_REFLECT_RESOURCE_FLAG_UAV) {
                if (binding->decoration_flags & SPV_REFLECT_DECORATION_NON_WRITABLE) {
					bindingInfo.access = UAVAccess::ReadOnly;
				}
                else if (binding->decoration_flags & SPV_REFLECT_DECORATION_NON_READABLE) {
					bindingInfo.access = UAVAccess::WriteOnly;
                }
            }

            
            bindingInfo.size = binding->block.size;
            bindingInfo.shaderVisibleStage = (uint16_t)shader->shaderStage;
            bindings.emplace_back(std::move(bindingInfo));
        }
        shader->reflectInfo = std::move(bindings);
        shader->inputAttributes = std::move(attributes);

    }
}
