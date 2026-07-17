#version 450
#include "ShaderResource.inl"
#include "BindlessSet.inl"
#include "imgui.fwd"

layout(location = 0) out vec2   o_pos;
layout(location = 1) out vec2   o_uv;
layout(location = 2) out vec4   o_color;
layout(location = 3) flat out int o_texId;
layout(location = 4) flat out vec4 o_scissor;

void main(){
    int curIndexIdx = gl_VertexIndex;
    int curVtxIdx   = GetBuffer(u_indexList).i[curIndexIdx];
    PerVtxData curVtxData = GetBuffer(u_vertexList).v[curVtxIdx];
    o_uv = vec2(curVtxData.i_uvx, curVtxData.i_uvy);
    uint  colorU32 = curVtxData.i_color;
    float colorr = colorU32 & 0xFF;
    colorU32 = colorU32 >> 8;
    float colorg = colorU32 & 0xFF;
    colorU32 = colorU32 >> 8;
    float colorb = colorU32 & 0xFF;
    colorU32 = colorU32 >> 8;
    float colora = colorU32 & 0xFF;

    vec4 color = vec4(colorr, colorg, colorb, colora) * (1.f / 255.f);
    o_color = color;
    //Each tri shares the same texture
    //Each tri made of 3 vertices
    uint curTriangleIdx = uint(curIndexIdx / 3);
    int attrIdx = GetBuffer(u_triAttIdxList).t[curTriangleIdx];
    Attribute att = GetBuffer(u_attrList).attr[attrIdx];
    vec2 displayScale = vec2(att.displayScaleX, att.displayScaleY);
    vec2 displayOffset = vec2(att.displayOffsetX, att.displayOffsetY);
    o_texId = int(att.texIdx);
    vec2 pos = vec2(curVtxData.i_posx, curVtxData.i_posy);
    pos = pos * displayScale + displayOffset;
    o_pos = pos;
    o_scissor = vec4(att.scissorXMin, att.scissorYMin, att.scissorXMax, att.scissorYMax);
    gl_Position = vec4(pos, 0.0, 1.0);
}