#version 450
//Out ---> a big triangle that cover's the full screen.     

layout(location = 0) out vec2 o_uv;
layout(location = 1) out vec2 o_pos;

void main() {
    vec2 pos;
    if (gl_VertexIndex == 0) {
        pos = vec2(-1.0, -1.0);
    } else if (gl_VertexIndex == 1) {
        pos = vec2(3.0, -1.0);
    } else {
        pos = vec2(-1.0, 3.0);
    }
    o_uv    = pos * 0.5 + 0.5;
    o_pos   = pos; 
    gl_Position = vec4(pos, 1.0, 1.0);
}