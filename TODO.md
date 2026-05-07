1. Render Path:   
   1. 其实我觉得前向很好，Forward+试一下？ 
   2. 算了还是先前向跑好了再说吧.. 先切到PBR    ✔   
   3. 天空盒和IBL   
   4. 透明支持     
   5. 其实可以延迟的。
   6. 多加几个后处理。  
   7. gpu driven剔除
2. 相机: 
   1. 做个正经FPS相机吧我受不了了。      ✔
3. Scene: 
   1. 尝试运行一个大场景 - sponza？           
   2. 相机剔除问题，场景管理器 （aabb/oct tree）                
   3. 序列化问题                 
4. api改进：             
   1. bindless texture             ✔
   2. bindless vertex input        ✔ (Not tested but should work as BDA is verified)   
   3. bindless everything.           
   4. 让渲染线程跑起来           
   5. msaa？                        
5. feature:                      
   1. 特别想尝试地形，做个简单的terrain？ gpu driven.         
      1. 几何方案               
         1. 高度图 RVT吧，虽然感觉没这个必要          
         2. cd lod                    
      2. 渲染方案            
         1. 洛克王国那种？其实就是id图+混合，没什么特别                  
         2. 就做个简单的得了，没那么多资产              
   2. 水     
      1. 几何                     
         1. FFT？
         2. 简单交互（似乎和fft不兼容         
      2. 渲染            
         1. 就那样吧         
         2. 打住            
   3. 骨骼？         
      1. 动画相关          
   4. 物理模拟               
      1. 再议      
   5. 介质渲染             
   6. 看一下原生光线追踪              
   7. streaming texture --- 似乎不是很难                 
   8. 打住，打住        
 

 BINDLESS WIP:    
 1. Barrier 统计优化： 加入一项PendingState，如果PendingState != resourceState，就说明这个resource在之前被绑定过，资源layout/barrier还未放置；代替现在的用std::set去重    
 2. Bindless 绑定部分：把buffer相关的部分改了，bindless不要用到offset和size，在shaderUBO中看看能不能反射出来，声明的时候需要一并声明这三个东西...
 3. Bindless 绑定部分：完成全部。
 4. 对Var binding count的特殊处理，以及shader反射时需要检测合法（必须是set中最后一个binding）
 5. VirtualPipeline中需要手动创建这个descriptorSet，整个Render应该只有有限个(或者是1个)

 1. 目前修改后，非bindless下没问题，除了一个小地方：
    - 现在shader参数的绑定分了俩个级别 -> 第一个是Entity级别的绑定，每个RenderEntity可以有自己的参数；第二个是material级别，每个material拥有自己的参数
    - Material的参数由谁来提交？毕竟只要提交一次，什么时候提交？
    - 参数提交就是上传到DescriptorSet里，现在这一步发生在DrawIndexed前，并且是对每个Entity做一次，这肯定不对。
    - Bindless的数据可能需要每帧去更新。    
    - Material里的绑定是不是可以有脏机制？虽然Material绑定开销很小（因为防止重复绑定问题DrawData里做了，所以API调用只会有一次）   
  2. 现在的Pass提交：把material上绑定好了所有要渲染的pass，然后放进RenderQueue里。之后每个Renderpass去queue里拿pass --- 这个得修改掉 -> 改成提交到renderQueue的不同pass的bucket里。然后每个RenderPass拿bucket，渲染。 
  3. 
 测试Bindless机制
