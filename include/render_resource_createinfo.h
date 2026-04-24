#ifndef RENDER_RESOURCE_CREATE_INFO
#define RENDER_RESOURCE_CREATE_INFO
#include "common/Name.h"
#include "render_resource_def.h"
#include <string>
#include <Vector>
#include <functional>
#include "limits.h"
namespace Render {

    struct BackEndInitDesc {
        std::string appName;
        std::string engineName;
        bool enableValidation = true;
        bool asyncTransferCompute = false;
    };

    struct BufferDesc {
        size_t byteSize;
        uint32_t bufUsage;
        uint8_t queueType;
        bool mappable = false;
    };

    struct ImageDesc {
        uint16_t        width;
        uint16_t        height;
        uint16_t        depth = 1;
        uint16_t        mipLevels = 1;
        uint16_t        arrayLayers = 1;
        ImageType       type = ImageType::V2D;
        ImageFormat     format;
        uint32_t        usage;
        SampleCount samples = SampleCount::Count1;
    };

    struct SamplerDesc {
        AddressMode     addressU = AddressMode::Repeat;
        AddressMode     addressV = AddressMode::Repeat;
        AddressMode     addressW = AddressMode::Repeat;
        Filter          minFilter = Filter::Linear;
        Filter          magFilter = Filter::Linear;
        MipMapMode      mipmapMode = MipMapMode::Linear;
        bool            enableAnisotropy = false;
        float           maxAnisotropy = 1.0f;
        CompareOp       compareOp = CompareOp::Never;   // 用于 Shadow samplers
        bool            unnormalizedCoords = false;
        BorderColor     borderColor = BorderColor::FloatOpaqueBlack;
        bool            enableCompare = false;
    };

    struct ShaderDesc {
        const char* shaderCode;
        const char* entryPoint = "main";
        size_t codeSizeByte = 0;
        ShaderStage   stage;       // 阶段
        bool isSpirv = false;
        struct ShaderCompileDesc* compileDesc = 0;
    };

    struct CommandBufferDesc {
        QueueType queueType;
        bool transient = false; //Will buffers be used multiTimes in oneframe?
        bool isSecondary = false;
    };

    enum class StorageOp : uint8_t {
        Cached,   
        Clear,      
        DontCare  
    };

    struct PassAttachment {
        RenderTextureFormat fmt = RenderTextureFormat::Invalid;
        SampleCount         SampleCount = SampleCount::Count1;
        StorageOp           loadOp;
        StorageOp           storeOp;
        bool isHDR = false; //float rt is HDR
    };

    struct ImageViewDesc {
        ViewAspect aspect        = ViewAspect::Color;
        uint16_t   mipLevel      = 0;
        uint16_t   arrayLayerBeg = 0;
        uint16_t   arrayLayerEnd = 0;
    };

    struct PassDesc {
        std::vector<PassAttachment> attachments;
        bool lastDepth = false;
        bool writeDepth = true;
    };

    struct BlendState {
        bool        blendEnable = false;
        BlendFactor srcColorBlend = BlendFactor::One;
        BlendFactor dstColorBlend = BlendFactor::Zero;
        BlendOp     colorBlendOp = BlendOp::Add;
        BlendFactor srcAlphaBlend = BlendFactor::One;
        BlendFactor dstAlphaBlend = BlendFactor::Zero;
        BlendOp     alphaBlendOp = BlendOp::Add;
    };

    struct RenderState {
        // 光栅化
        bool        depthTestEnable = true;
        bool        depthWriteEnable = true;
        CompareOp   depthCompareOp = CompareOp::Less;

        // 模板测试
        bool        stencilTestEnable = false;                  // 是否启用模板测试
        CompareOp   stencilCompareOp = CompareOp::Always;      // 模板比较操作
        StencilOp   stencilFailOp = StencilOp::Keep;        // 模板比较失败后的操作
        StencilOp   stencilPassOp = StencilOp::Keep;        // 模板比较与深度测试都通过后的操作
        StencilOp   stencilDepthFailOp = StencilOp::Keep;        // 模板比较通过但深度测试失败后的操作
        uint32_t    stencilReadMask = 0xFF;                   // 模板比较掩码
        uint32_t    stencilWriteMask = 0xFF;                   // 模板写入掩码
        uint32_t    stencilReference = 0;                      // 模板参考值

        std::vector<BlendState> blendStates;
        
        Topology topology = Topology::TriangleList;
        FillMode fillMode = FillMode::Fill;
        CullMode cullMode = CullMode::None;
        FrontFace frontFace = FrontFace::ClockWise;

        // 多重采样
        uint32_t    rasterizationSamples = 1;
    };

    //Binding Position is the declared sequence in pipeline buffer list
    struct InputBufferBinding {
        uint32_t    stride;        // bytes per vertex / instance
        bool        perInstance;   // false = per-vertex, true = per-instance
    };

    struct InputAttribute {
        uint32_t        location;      // shader location
        uint32_t        binding;       // binding index
        VertexFormat    format;
        uint32_t        offset;        // byte offset
    };

    struct VertexInputDescription {
        std::vector<InputBufferBinding>   bindings;
        std::vector<InputAttribute> attributes;
    };
    struct rs_shader_module;
    struct PipelineDesc {
        PipelineType         type;         // Graphics / Compute / RayTracing
        std::vector<rs_shader_module*> shaders;
        RenderState          renderState;  // 仅 Graphics 有效
        VertexInputDescription vertexInputDesc;
    };

#define INVALID_BINDING_POS UINT_MAX
    struct BindingInfo {
        Name bindingItemName;
        rs_binding_pos bindingPos; //Platform related
        uint16_t shaderVisibleStage = 0; //shader stage
        uint16_t count = 0; 
        uint16_t size = 0; 
        UniformType type;
        ImageType   imageType = ImageType::Invalid;
        UAVAccess   access =    UAVAccess::ReadOnly;           //For Storagebuffer and StorageImage
    };

    struct Rect2D {
        float l = 0.0f, r = 1.0f, t = 0.0f, b=1.0f;
    };

    struct VertexBindingInfo {
        uint16_t offset;
        struct rs_buffer* buffer;
    };

	union ImageViewKey {
	private:
		struct BitField {
			uint64_t aspect : 4;  // [0-3]
			uint64_t viewType : 4;  // [4-7]
			uint64_t baseMip : 6;  // [8-13]
			uint64_t mipCount : 6;  // [14-19]
			uint64_t baseLayer : 6;  // [20-25]
			uint64_t layerCount : 6;  // [26-31]
			uint64_t reserved : 32; // [32-63]
		};
	public:
		BitField bits;
		uint64_t value;

	public:
		ImageViewKey();
		ImageViewKey(uint64_t rawValue);

		ImageViewKey& setAspect(ViewAspect aspect);
		ImageViewKey& setViewType(ImageType type);
		ImageViewKey& setBaseMip(uint32_t mip);
		ImageViewKey& setMipCount(uint32_t count);
		ImageViewKey& setBaseLayer(uint32_t layer);
		ImageViewKey& setLayerCount(uint32_t count);

		ViewAspect getAspect() const;
		ImageType  getViewType() const;
		uint32_t   getBaseMip() const;
		uint32_t   getMipCount() const;
		uint32_t   getBaseLayer() const;
		uint32_t   getLayerCount() const;

		bool operator==(const ImageViewKey& other) const;
		bool operator!=(const ImageViewKey& other) const;

		bool isValid() const;
	};

    struct RenderInfo {
        std::vector<VertexBindingInfo > bindingBuffers;                      //in buffers
        struct rs_buffer* indexBuffer = 0;
        IndexType indexType = IndexType::Uint32;
        uint32_t idxOffset = 0;
        uint32_t idxCount = 0;
        uint32_t vtxoffset = 0;
        uint32_t instanceCount = 1;
        bool isIndirect = false;
    };

    struct ShaderIncludeRes {
        bool FindResult;
        std::string ShaderName;
        std::string ShaderContent;
    };

    using ShaderIncFindFunc = std::function<ShaderIncludeRes(const std::vector<std::string>&, const std::string&)>;

    struct ShaderMacroPair {
        std::string Name;
        std::string Value;
    };
    using MacroPairs = std::vector<ShaderMacroPair>;
    using StageMacroPairs = std::vector<std::pair<ShaderStage, MacroPairs>>;
    struct ShaderCompileDesc {

        
        ShaderStage stage;
        ShaderLang langType = ShaderLang::GLSL;
        std::string shaderName;
        std::string shaderSrcCode;
        bool enableOptimize = false;
        bool generateDebugInfo = false;
        MacroPairs macros;
        std::vector<std::string> shaderIncludeDirectories;
        ShaderIncFindFunc shaderIncludeFindFunc = nullptr;
    };

    enum class ResourceLocationType {
        BindingSlot,
        BindlessSlot
    };

	struct ResourceLocation {
        ResourceLocationType type;
        Name             itemName;
		rs_binding_pos   bindingPos;
		union {
			struct {
				Render::UniformType type; 
				uint16_t            count; 
			    uint16_t            size;
            } descriptorInfo;

			struct {
				uint16_t offset; 
				uint16_t count;  
				uint16_t stride; 
			} bindlessInfo;
		};
	};

	using ShaderStageInfo = std::vector<std::pair<ShaderStage, std::string>>;

};

#endif