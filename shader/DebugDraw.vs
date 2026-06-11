#version 450

#include "CommonSets.inl"
#include "CommonMath.inl"
layout(location = 0) in vec3 i_pos;         
   
layout(location = 1) in vec4 i_perObjWorld0;            
layout(location = 2) in vec4 i_perObjWorl1;            
layout(location = 3) in vec4 i_perObjWorld2;            
layout(location = 4) in vec4 i_perObjWorld3;     

layout(location = 5) in vec4 i_perObjColor;            


layout(location = 6) in vec3  i_normal;            
layout(location = 7) in float i_usebillboard;            

layout(location = 0) out vec4 o_color;

void main(){
    mat4 i_perObjWorld = mat4(i_perObjWorld0, i_perObjWorl1, i_perObjWorld2, i_perObjWorld3);
    
    vec4 worldPos = (i_perObjWorld) * vec4(i_pos, 1.0f);

    if (i_usebillboard > 0.5) {
        vec3 centerPos = i_perObjWorld[3].xyz;
        vec3 offset = worldPos.xyz - centerPos;
        vec3 worldNormal = normalize(mat3(i_perObjWorld) * i_normal);
        
        vec3 viewDir = normalize(CAMDATA.CameraPosition.xyz - centerPos);

        float angle = GetVecAngle(worldNormal, viewDir);

        vec3 rotAxis = cross(worldNormal, viewDir);

        if (length(rotAxis) > 0.001) {
            offset = RotateAroundAxis(offset, rotAxis, angle);
        } 
        else if (dot(worldNormal, viewDir) < -0.99) {
            vec3 arbitraryAxis = cross(worldNormal, vec3(0.0, 1.0, 0.0));
            if (length(arbitraryAxis) < 0.001) {
                arbitraryAxis = cross(worldNormal, vec3(1.0, 0.0, 0.0));
            }
            offset = RotateAroundAxis(offset, arbitraryAxis, 3.14159265);
        }
        worldPos.xyz = centerPos + offset;
    }
    
    o_color = i_perObjColor.xyzw;
    gl_Position = CAMDATA.MatViewProj * worldPos;
}