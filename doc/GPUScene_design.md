## GPU Scene    
GPU Scene主要用来做GPU Driven的   
需要在一次drawcall内能完全访问到    
目前我的设计是   
在场景导入到渲染器的时候直接做一次转化，这个转化和普通的Node差不多，唯一区别是会创建一个Component把自己注册到一个GPUSceneManager里去    
这个Manager会维护几个SSBO，分别存 VertexBuffer的BDA的array，index Buffer BDA Array, Drawcall info Array, MaterialIndex array， Material Array   
     
