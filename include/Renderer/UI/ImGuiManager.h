#ifndef IMGUI_MANAGER_H_
#define IMGUI_MANAGER_H_

#include "function/InputDef.h"
#include "common/Singleton.h"

namespace Render {
	struct rs_image_view;
	struct rs_commandbuffer;

	class IMGUIManager :public Singleton<IMGUIManager>{
	public:
		IMGUIManager();
		
		//Mostly handle render related in this func
		void draw();
		//Mostly handle IO in this func
		void update();
		//0-100
		void setImGuiScrollSensitivity(float x);
		//After render passes created.
		void init();
		void deinit();
		~IMGUIManager();
		void setTextureSamplerFilter(Filter filter);
	private:
		class ImGuiManagerPrivate* mDP;
	};
}
#endif //!IMGUI_MANAGER_H_