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
   1. bindless texture             
   2. bindless vertex input            
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
 