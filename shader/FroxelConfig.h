#include "ShaderResource.inl"
DECL_BUFFER_STD430_BEG(FroxelConfigData)
    mat4 viewMat;
    mat4 ProjMat;
    mat4 invProjMat;
    mat4 invViewProjMat;
    vec3 camPosition;
    float screenSizeX;
    float screenSizeY;
    float specialNear;
    float camNear;
    float camZFar;
    int tileXMax;
    int tileYMax;
    int tileZMax;
DECL_BUFFER_STD430_END
