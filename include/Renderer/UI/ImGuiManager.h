#ifndef IMGUI_MANAGER_H_
#define IMGUI_MANAGER_H_

#include "common/Singleton.h"


namespace Render {
	struct rs_image_view;
	struct rs_commandbuffer;

	ImTextureID toImTex(rs_image_view* view);

	class IMGUIManager{
	public:
		IMGUIManager();
		
		void draw(rs_commandbuffer* cmd);

		~IMGUIManager();

	private:
		class ImGuiManagerPrivate* mDP;
	};
}
#endif //!IMGUI_MANAGER_H_