#ifndef DEBUG_PRINT_INL_H
#define DEBUG_PRINT_INL_H

#if defined(SHADER_DEBUG_PRINT) && defined(DRAW_META_DATA) && defined(BINDLESS_ENABLE) && defined(DEBUG_PRINT) && !defined(COMPUTE)

#extension GL_EXT_buffer_reference : require
#extension GL_EXT_shader_explicit_arithmetic_types_int64 : require

#define DEBUG_PRINT_FLAG_COUNT 4096

//A device-address SSBO with one flag per draw-call slot. The base address rides
//along in the push-constant metadata (see GET_DEBUG_FLAG_BUFFER_ADDRESS in
//ShaderDrawMetaData.inl); the RHI layer owns the buffer and resets it each frame.
//This is what lets a shader print exactly once per draw instead of once per
//fragment.
layout(buffer_reference, std430) buffer DebugFlagBuffer {
    uint printedFlag[DEBUG_PRINT_FLAG_COUNT];
};

//Returns true only for the first invocation that manages to claim this draw-call
//slot. Cheap non-atomic peek first so that, once printed, every later fragment
//just does a load and returns.
bool debugPrintOnceSlot(uint slot) {
    uint64_t addr = GET_DEBUG_FLAG_BUFFER_ADDRESS();
    if (addr == uint64_t(0)) {
        return false;
    }
    DebugFlagBuffer flags = DebugFlagBuffer(addr);
    slot = slot % uint(DEBUG_PRINT_FLAG_COUNT);
    if (flags.printedFlag[slot] != 0u) {
        return false;
    }
    return atomicCompSwap(flags.printedFlag[slot], 0u, 1u) == 0u;
}

//Once per shader (pipeline) per frame.
bool debugPrintOnce() {
    return debugPrintOnceSlot(GET_PIPELINE_INDEX());
}

//Once per draw call per frame.
bool debugPrintOnceDraw() {
    return debugPrintOnceSlot(GET_DRAW_CALL_INDEX());
}

#define DEBUG_PRINT_ONCE(args) do { if (debugPrintOnce()) { DEBUG_PRINT(args); } } while(false)
#define DEBUG_PRINT_ONCE_DRAW(args) do { if (debugPrintOnceDraw()) { DEBUG_PRINT(args); } } while(false)

#else
#define DEBUG_PRINT_ONCE(args)
#define DEBUG_PRINT_ONCE_DRAW(args)
#endif //graphics debug print available

#endif //DEBUG_PRINT_INL_H