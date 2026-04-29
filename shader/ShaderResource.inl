#ifdef ENABLE_BINDLESS
#define DECL_BUFFER_STD140_BEG(type_name) layout(buffer_reference, std140) buffer type_name {
#define DECL_BUFFER_STD140_END()          };

#define DECL_BUFFER_STD430_BEG(type_name) layout(buffer_reference, std430) buffer type_name {
#define DECL_BUFFER_STD430_END()          };
#define NAME_TEX(name)    tex_##name
#define NAME_RWTEX(name)  rwtex_##name
#define NAME_SAMP(name)   samp_##name
#define NAME_BUF(name)    buf_##name

#define RESOURCE_DECL_BEG(s) layout(set = s) uniform BINDLESS_##s {
#define RESOURCE_DECL_END };

#define SLOT_BUFFER_STD140(s, type, name) type NAME_BUF(name);
#define SLOT_BUFFER_STD430(s, type, name) type NAME_BUF(name);

#define SLOT_TEXTURE(s, type, name)       uint NAME_TEX(name);
#define SLOT_RWTEXTURE(s, type, name)     uint NAME_RWTEX(name);
#define SLOT_SAMPLER(s, type, name)       uint NAME_SAMP(name);

#define GetBuffer(name)                      NAME_BUF(name)

#define GetRWTexture(name)                   GlobalUAVImages[NAME_RWTEX(name)]
#define GetTexture(name)                     GlobalTextures[NAME_TEX(name)]
#define GetSampler(name)                     GlobalSamplers[NAME_SAMP(name)]

#define GetSampledTexture(texName, sampName) sampler2D(GlobalTextures[NAME_TEX(texName)], GlobalSamplers[NAME_SAMP(sampName)])
#define SampleTexture(texName, sampName, uv) texture(sampler2D(GlobalTextures[NAME_TEX(texName)], GlobalSamplers[NAME_SAMP(sampName)]), uv)

#else

#define DECL_BUFFER_STD140_BEG(type_name) struct type_name {
#define DECL_BUFFER_STD140_END()          };

#define DECL_BUFFER_STD430_BEG(type_name) struct type_name {
#define DECL_BUFFER_STD430_END()          };

#define NAME_TEX(name)    name
#define NAME_RWTEX(name)  name
#define NAME_SAMP(name)   name
#define NAME_BUF(name)    name

#define RESOURCE_DECL_BEG(s) 
#define RESOURCE_DECL_END 

#define SLOT_BUFFER_STD140(s, type, name) layout(set = s, std140) buffer block_##name { type NAME_BUF(name); };
#define SLOT_BUFFER_STD430(s, type, name) layout(set = s, std430) buffer block_##name { type NAME_BUF(name); };

#define SLOT_TEXTURE(s, type, name)       layout(set = s) uniform type NAME_TEX(name);
#define SLOT_RWTEXTURE(s, type, name)     layout(set = s) uniform type NAME_RWTEX(name);
#define SLOT_SAMPLER(s, type, name)       layout(set = s) uniform type NAME_SAMP(name);

#define GetBuffer(name)                      NAME_BUF(name)

#define GetRWTexture(name)                   NAME_RWTEX(name)
#define GetTexture(name)                     NAME_TEX(name)
#define GetSampler(name)                     NAME_SAMP(name)

#define GetSampledTexture(texName, sampName) sampler2D(NAME_TEX(texName), NAME_SAMP(sampName))
#define SampleTexture(texName, sampName, uv) texture(sampler2D(NAME_TEX(texName), NAME_SAMP(sampName)), uv)

#endif