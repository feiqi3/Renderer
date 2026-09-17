#ifndef SHADER_EXTENSIONS_INL_H
#define SHADER_EXTENSIONS_INL_H
#extension GL_KHR_shader_subgroup_basic : enable
#if defined(BUFFER_DEVICE_ADDRESS)

#extension GL_EXT_shader_explicit_arithmetic_types_int64 : enable
#define ADDRESS64(x) uint64_t(x)

#else

#define ADDRESS64(x)

#endif//BUFFER_DEVICE_ADDRESS

#if defined(SHADER_DEBUG_PRINT)
    #extension GL_EXT_debug_printf : enable
    //Use double brackets!!!!!!!
    #define DEBUG_PRINT(args) debugPrintfEXT args
#else
    #define DEBUG_PRINT(args) 
#endif//SHADER_DEBUG_PRINT

#endif //SHADER_EXTENSIONS_INL_H