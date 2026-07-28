# 多光源问题     

对于Forward来说要遍历所有光源列表去计算光照着色问题挺大的    
在Doom2016中的Forward+倒是可以使用一下（但是不要它的ThinGBuffer，没p用）    
把视锥体划分成Voxel，然后计算每个Voxel里有什么光源（后续可能还有体积光相关）     
有些光源会因为遮挡的缘故不再可见 --- 使用HiZ做剔除   
因为要做HiZ所以需要PreZ，这里又可以在PreZpass去计算MotionVector     
简单的GPU-driven 光照      
