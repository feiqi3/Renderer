# ImGuiManager
用于利用现有的RHI绘制ImGUI的控件   
其实我不太喜欢IMGUI自带的Backend实现，委实有些低效。   
一个drawcall就是一个quad加上一个texture
其实可以用bindless做的特别高效：
一个instance drawcall，用idx索引quad的坐标，用idx索引texture...bindless！    
每帧只需要做两次buffer的同步，一个vtxbuffer，一个textureidxbuffer      
