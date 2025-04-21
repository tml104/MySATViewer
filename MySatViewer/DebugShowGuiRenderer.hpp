#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "RenderInfo.hpp"
#include "IRenderable.hpp"

#include "shader_s.h"

#include "DebugShowInfo.hpp"

#include "SetCameraPosEvent.hpp"
#include "Dispatcher.hpp"

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

namespace MyRenderEngine {

	class DebugShowGuiRenderer : public IRenderable {
	public:
		Info::DebugShowInfo& debugShowInfo;
		
		void RenderDebugShowPointInfos() {
			if (ImGui::TreeNode("DebugShow Points")) {
				int id = 0;
				for (Info::DebugShowPointInfo& point_info : debugShowInfo.things.pointInfos) {
					ImGui::PushID(id++);
					if (ImGui::TreeNode("", "Point: %s", point_info.name.c_str())) {
						ImGui::Text("Pos: (%f, %f, %f)", point_info.pos.x, point_info.pos.y, point_info.pos.z);
						ImGui::Text("Color: (%f, %f, %f)", point_info.color.x, point_info.color.y, point_info.color.z);

						if (ImGui::Button("GO")) {
							// 设置相机位置的事件

							EventSystem::SetCameraPosEvent e{ point_info.pos };
							EventSystem::Dispatcher::GetInstance().Dispatch(e);

						}


						ImGui::TreePop();
					}
					ImGui::PopID();
				}
				ImGui::TreePop();
			}

		}

		void Render(
			const RenderInfo& renderInfo
		) override {

			ImGui::Begin("Debugshow Info");

			RenderDebugShowPointInfos();

			ImGui::End();
		}

		
		DebugShowGuiRenderer(Info::DebugShowInfo& debugShowInfo) : debugShowInfo(debugShowInfo) {}

		~DebugShowGuiRenderer() {}

	};

}