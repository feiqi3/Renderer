#version 450
#include "ShaderResource.inl"
#include "BindlessSet.inl"
#include "imgui.fwd"
struct PerVtxData{
    vec2 i_pos;
    vec2 i_uv;
    vec4 i_color;
}


layout(location = 0) out vec2   o_pos;
layout(location = 1) out vec2   o_uv;
layout(location = 2) out vec4   o_color;
layout(location = 3) out int    o_texId;


void main(){
    int curIndexIdx = gl_VertexIndex;
    int curVtxIdx   = GetBuffer(u_indexList).i[curIndexIdx];
    PerVtxData curVtxData = GetBuffer(u_vertexList).v[curVtxIdx];
    o_pos = curVtxData.i_pos;
    o_uv = curVtxData.i_uv;
    o_color = curVtxData.i_color;
    //Each quad shares the same texture
    //Each quad made of 6 vertices
    o_texId = GetBuffer(u_textureList).t[int(gl_VertexIndex / 6)];
    vec2 pos = curVtxData.i_pos;
    pos = pos * 2.0 - 1.0;
    gl_Position = vec4(pos, 0.0, 1.0);
}