# 1. 动机    
绘制阴影用    

## 1. 平行光阴影     
只在ShadowCamera里绘制当前Camera能看到的那些物体    
如何计算这个ShadowCamera的view呢？    
用ProjectionSpace的八个点过一边InvViewProj，就能得到世界空间中当前的Frustum的几个点    
用这个Frustum放入一个AABB里，这个也就是DirLightShadowCamera应该能看到的地方    
接下来，把Camera设置为Ortho，然后把看的中心放在AABB的中心，接着把这个ortho的size设置为AABB的length的xy，这样就能保证这ShadowMap能覆盖到主相机能看到的所有东西    

```cpp   
		float zDepthMin, zDepthMax;
		RenderSystem::instance()->getGlobalViewportZRange(zDepthMin, zDepthMax);

		float farZ = currentCamera->getFar();
		const float FarestDistanceDirShadow = 100.0f;
		float shadowFarZ = std::min(FarestDistanceDirShadow, farZ);

		vec4 viewPointFar(0.0f, 0.0f, -shadowFarZ, 1.0f);
		vec4 clipPointFar = currentCamera->getProjectionMatrix() * viewPointFar;
		float ndcFarZ = clipPointFar.z / clipPointFar.w;

		float ndcNearZ = zDepthMin; 

		const vec4 NDCCoord[8] = {
			vec4(-1.0f, -1.0f, ndcNearZ, 1.0f),
			vec4(1.0f, -1.0f, ndcNearZ, 1.0f),
			vec4(1.0f,  1.0f, ndcNearZ, 1.0f),
			vec4(-1.0f,  1.0f, ndcNearZ, 1.0f),
			vec4(-1.0f, -1.0f, ndcFarZ,  1.0f),
			vec4(1.0f, -1.0f, ndcFarZ,  1.0f),
			vec4(1.0f,  1.0f, ndcFarZ,  1.0f),
			vec4(-1.0f,  1.0f, ndcFarZ,  1.0f)
		};

		AxisAlignedBoundingBox aabbOfFrustum;
		float farPlaneZ	 = clamp(FarestDistanceDirShadow / currentCamera->getFar(),0.1f,1.f);
		auto viewProjOfCurCam = currentCamera->getProjectionMatrix() * currentCamera->getViewMatrix();
		auto invViewProj = inverse(viewProjOfCurCam);
		for (int i = 0;i < 8;++i) {
			vec4 PointNDCCoord = NDCCoord[i];
			vec4 worldPoint = invViewProj * PointNDCCoord;
			worldPoint = worldPoint / worldPoint.w;
			aabbOfFrustum.expand(worldPoint);
		}

		float radius = length(aabbOfFrustum.getSize()) / 2.;
		vec3 camPos = aabbOfFrustum.getCenter() - light->getDirection() * radius;
		DebugDrawManager::instance()->drawAABB(aabbOfFrustum,vec4(1,.0,0,0.3));
		float nearPlane = 0.01f;
		float farPlane = radius * 2.0f;
		mDp->mDirLightCamera->setTarget(aabbOfFrustum.getCenter());
		mDp->mDirLightCamera->setOrthoSize(radius);
		mDp->mDirLightCamera->setOrthographic(radius, 1., nearPlane, farPlane);
```     
另一个值得注意的地方就是，为了限制ShadowDirLightCamera可以看到的最远的地方，所以需要对NDCCoord.z做出限制。     
现在View空间中，放一个中心点，vec4(0,0,FarZ,1)，然后换到ProjectionSpace，这才是用来反计算frustumAABB的坐标的NDCCoord    
之所以需要先乘以Projection而不直接用ShadowDirCameraFarZ/CurrentCameraFarZ得到这个NDC的Z，是因为这个Z在Projection是非线性的。      

NDC空间的范围是 -1，1 但是z不一定是，ndc的z是可以通过viewport来设置的。          

### 问题：   
1. 阴影抖动？    ---- 在viewSpace计算shadowCamera的boundingSphere大小
2. 阴影RT分辨率不足，导致阴影太颗粒了....  ----现在是2048*2048画sponza，如果不行只能继续做csm咯？     


### 2. CSM   
用一个TextureArrayLayer拼接所有的ShadowMap    
texture2dArray[0]负责相机空间中深度为0-5，texture2dArray[1]负责相机空间中深度为5-30，texture2dArray[2]负责相机空间中深度为30-150，    

在CPP中，创建数个阴影相机，创建一个Texture2DArray，array的每个layer作为RT的depthAttachment，用这几个阴影相机做剔除，复用一个RenderPass，只是绑定不同的RT，就可以完成绘制了    

shader中，每次计算出当前所属于的layer，然后采样，得到shadowVisible就可以了    
csm过渡：用一个Factor控制过渡范围   
当factor=0.1，上一级CascadedLayer的范围是0-10的时候，这个过渡范围就是CameraDepthZ从9-10的时候    
然后做一次采样下一级阴影图，做一个平滑过渡就可以了      


## 2. Filter     
PCF，采样数个点，对每个点计算ShadowVis，对ShadowVis做平均   
PCSS，没看到什么必要      

## 3. 点光源阴影    
TODO:        
### 1. 正方形   
用一个TextureCube的每个面作为RT渲染   

### 2. 四面体    
map到一个四面体的四个面上进行渲染     

### 3. 双曲   
map到两个鱼眼镜头上进行渲染   

