namespace Render{
	//This could be wrong when doing something across multi device...But who cares
	inline struct rs_image* defalut_no_texture = nullptr;
	inline struct rs_image* defalut_no_texture_UAV = nullptr;
	inline struct rs_sampler* defalut_no_sampler = nullptr;
	inline struct rs_buffer* defalut_no_buffer = nullptr;
	inline struct rs_buffer* defalut_no_buffer_UAV = nullptr;
	inline uint32_t default_bindless_texture_idx	= 0;
	inline uint32_t default_bindless_textureuav_idx = 0;
	inline uint32_t default_bindless_sampler_idx	= 0;
	inline uint64_t default_bindless_buffer_idx = 0;
}