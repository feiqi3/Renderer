1. camera class -> 渲染开始时设置camera，camera设置全局的渲染common set   
2. 真正的maincam pass -> 深度 + color attachment    
3. UI系统？

（WIP，快速推进，一个月内，每天2h）
4. 分离RenderPass和RenderTarget ---> RenderTarget格式，目前需要在创建Variant时手动输入，有点复杂 --> 解决：提供几套预设的Pass设置？
	分离pipelinelayout和pipeline，变成创建material其实只是编译shader，然后得到pipelinelayout和一个占位的pipeline   
	当真正渲染的时候，renderpass中会把RenderEnity送入queue，在开始记录命令时开始查找这个material下面对应的pipeline，怎么查找？当前的把当前renderpass的所有rt texture的格式/Samples编码到一个u64上，然后查map，如果存在，就用，如果不存在，尝试创建PSO。   
	1. 在这种设计下，material现在是pipelinelayout和pipeline的结合体
	2. 创建一个material，实际上只创建了pipelinelayout。这里面记录了绑定信息，并且被用来创建descriptorSet相关。
	3. 这部分目前还在material里，未来会被挪到rhi层？或者是pipeline内部会有这么一个。坏处：放弃了 aot 的优势，但是可以用cache的方式弥补。 未来把shader code和macro + render pass属性哈希后保存。
