#ifdef ENABLE_BINDLESS

#define RESOURCE_DECL_BEG(s) layout(set = s) uniform BINDLESS_##s {
#define RESOURCE_DECL_END };

#define RESOURCE(name)  name
#define SLOT(s, type, name) uint RESOURCE(name);
#else

#define RESOURCE_DECL_BEG(s) 
#define RESOURCE_DECL_END 
#define RESOURCE(name)  name
#define SLOT(s, type, name) layout(set = s) uniform type RESOURCE(name);

#endif