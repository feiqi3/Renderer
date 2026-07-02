# 1. 动机    
为了减少vulkan绑定参数带来的开销，建立这个类   
其用意是，把一些常用的参数绑定集中于此   
对于vk来说就是几个DescriptorSet，在这里被集中绑定     
在这个项目中，DescriptorSet被抽象为了rs_drawdata，内部处理了绑定相关的细节    

对于camera，或者是scene，其参数是不会随着不同的渲染对象而变化的，完全可以在RenderFrame的开头绑定完成，但是又有着不同的绑定频率，所以需要几个不同的drawdata   
何为绑定频率不同？？一帧可能为了各种原因切换不同相机，但一帧切换两个场景（或者同时渲染两个场景）的情形比较少  
每次在RenderFrame里切换不同的相机，内部会自动调用`ConstShaderDataManager::updateCameraDrawData`，当切换场景的时候会自动调用`ConstShaderDataManager::updateSceneDrawData`    

这个设计本来可以沿用很久，但是有个问题出现了，“阴影”。   
在阴影Pass，此时是没有ShadowTexture的，到了MainCameraPass才会出现。如果在阴影Pass不绑定，就会出现Vulkan的“槽位无效”错误，如果在生成了之后进行绑定，就会导致BindAfterDraw问题。所以必然要用一个宏在shader内隔离，也就会导致DescriptorSetLayout不同，使得现下的绑定机制失效。

我这里有数个方案，但各有其问题：   
### 1.利用Material的绑定机制      
把现在的ConstShader绑定机制和Material的绑定机制同步，换而言之，也变成按照Pass来绑定。我有五个renderPass，我就创建五个Pass，然后依赖Material的绑定机制，对Scene和Camera的参数进行绑定。     
这样带来的问题是性能的极大下降，原本PerScene，PerCamera的绑定机制会退化到PerRenderPass，而一个RenderPass完全可能是PerScene+PerCamera的（取决于以后的设计），这个开销有些恐怖        

### 2.单独创建一个ShadowSet     
这个比较符合我心意，但是如果未来需要再加点什么，那就很糟糕了     


### 3.做一套脏DrawData机制       
如果发现ConstShaderData在绘制之前被update了，那就创建一个新的，销毁旧的。   
这个可以解决BindAfterDraw的问题。    

## 结论：   
先推进2.


