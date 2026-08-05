
int linearZToZBin(float zLinear, float spN, float near, float far, float binCount){
    float scale = (binCount - 1) / (-log2(
        (spN/far)))
        ;

    //From neg linear z to normalize z.
    float log2z = log2(-zLinear / far);
    int possibleBin = int(floor(max(log2z * scale + binCount, 0)));
    return possibleBin;
}

//Return z range is positive
vec2 binToZLinear(int bin, float spN, float near, float far, float binCount){
    
    float scale = (binCount - 1) / (- log2(spN/far));
    float maxBinZ = exp2((float(bin) - binCount + 1) / scale);
    float minBinZ = (bin == 0) ? 0 : exp2((float(bin) - binCount) / scale);
    return vec2(minBinZ,maxBinZ) * (far);
}

int getBinIndex(ivec3 froxelPos, int maxTileX,int maxTileY,int maxTileZ){
    return froxelPos.z * (maxTileX * maxTileY) + froxelPos.y * maxTileX + froxelPos.x;
}