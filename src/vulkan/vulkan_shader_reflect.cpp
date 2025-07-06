#include "vulkan/vulkan_shader_reflect.h"
#include "shaderc/shaderc.hpp"
#include "render_log.h"
#include "spirv_reflect.h"
#include <fstream>
#include <filesystem>
namespace {
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
                return ShaderIncludeRes{ path.string(), content };
            }
        }
        return ShaderIncludeRes{ "", "Error: include not found: " + requested_source };
    }
}

namespace Render::Vulkan {
	rs_shader_module_vk* compileShader(rs_context_vk* ctx, const ShaderCompileDesc& desc)
	{
		shaderc::CompileOptions options;
		shaderc::Compiler compiler;
        options.SetIncluder(std::unique_ptr<shaderc::CompileOptions::IncluderInterface>(new FunctionalIncluder(desc.shaderIncludeDirectories, desc.shaderIncludeFindFunc)));
        
        shaderc_optimization_level lvl = desc.enableOptimize ? shaderc_optimization_level_performance : shaderc_optimization_level_zero;

        options.SetOptimizationLevel(lvl);
        for (auto&& [name, key] : desc.macros) {
            options.AddMacroDefinition(name, key);
        }
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

        shaderc::SpvCompilationResult module =
            compiler.CompileGlslToSpv(desc.shaderSrcCode, MapShaderStageToShaderc(desc.stage),desc.shaderName.c_str());

        if (module.GetCompilationStatus() != shaderc_compilation_status_success) {
            Render::Log::error("Shader Compile Error: " + desc.shaderName + " \nError Reason: " + module.GetErrorMessage());
            return nullptr;
        }

        if (module.GetNumWarnings() > 0) {
            Render::Log::warn("Shader Compile Success with warning: " + desc.shaderName + " \Warning : " + module.GetErrorMessage());
        }
        VkShaderModule vkModule;
        VkShaderModuleCreateInfo ci{VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO};
        ci.codeSize = module.end() - module.begin();
        ci.pCode = module.begin();
        VK_CHECK(vkCreateShaderModule(ctx->device, &ci, 0, &vkModule), { return nullptr; });
        rs_shader_module_vk* shaderModule = new rs_shader_module_vk;
        shaderModule->native = vkModule;
        shaderModule->shaderName = desc.shaderName;
        shaderModule->shaderStage = desc.stage;

        return shaderModule;
	}
    void reflectShader(rs_shader_module_vk* shader)
    {
        //TODO:
    }
}
