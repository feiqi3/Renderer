#ifdef ENABLE_BINDLESS

#define RESOURCE_DECL_BEG(s) layout(set = s) uniform BINDLESS_##s {
#define RESOURCE_DECL_END };

#define NAME_SRV(name)  srv_##name
#define NAME_UAV(name)  uav_##name
#define NAME_SAMP(name) samp_##name
#define NAME_BUF(name)  buf_##name

#define SLOT_SRV(s, type, name)  uint NAME_SRV(name);
#define SLOT_UAV(s, type, name)  uint NAME_UAV(name);
#define SLOT_SAMP(s, type, name) uint NAME_SAMP(name);
#define SLOT_BUF(s, type, name)  uint NAME_BUF(name);

#define RESOURCE(name)  NAME_SRV(name)
#define SLOT(s, type, name) SLOT_SRV(s, type, name)

#else

#define RESOURCE_DECL_BEG(s) 
#define RESOURCE_DECL_END 

#define NAME_SRV(name)  srv_##name
#define NAME_UAV(name)  uav_##name
#define NAME_SAMP(name) samp_##name
#define NAME_BUF(name)  buf_##name

#define SLOT_SRV(s, type, name)  layout(set = s) uniform type NAME_SRV(name);
#define SLOT_UAV(s, type, name)  layout(set = s) uniform type NAME_UAV(name); 
#define SLOT_SAMP(s, type, name) layout(set = s) uniform type NAME_SAMP(name);
#define SLOT_BUF(s, type, name)  layout(set = s) buffer  type NAME_BUF(name);

#define RESOURCE(name)  NAME_SRV(name)
#define SLOT(s, type, name) layout(set = s) uniform type NAME_SRV(name);

#endif