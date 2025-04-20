#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "RenderInfo.hpp"
#include "IRenderable.hpp"

#include "shader_s.h"

#include "SatInfo.hpp"

#include "SetCameraPosEvent.hpp"
#include "Dispatcher.hpp"

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

namespace MyRenderEngine {

	class SatGuiRenderer : public IRenderable {
	public:
		Info::SatInfo& satInfo;

		// TODO
		void RenderVertexInfos() {

		}

		void RenderEdgeInfos() {
			if (ImGui::TreeNode("Edges")) {
				int id = 0;

				for (Info::EdgeInfo& edge_info : satInfo.brepInfo.edgeInfos) {

					// 对非流形编特殊着色
					bool color_flag = false;
					ImU32 treenode_color;
					if (edge_info.nonmanifoldCount == 1) {
						color_flag = true;
						treenode_color = IM_COL32(255, 0, 0, 255); // Red 
					}
					else if (edge_info.nonmanifoldCount > 2) {
						color_flag = true;
						treenode_color = IM_COL32(0, 0, 255, 255); // Blue
					}

					if (color_flag) {
						ImGui::PushStyleColor(ImGuiCol_Text, treenode_color);
					}

					ImGui::PushID(id++);

					if (ImGui::TreeNode("", "Edge: %d", edge_info.markNum)) {
						ImGui::Text("Nonmanifold Count: %d", edge_info.nonmanifoldCount);
						ImGui::Text("Body ID: %d", edge_info.bodyId);
						ImGui::Text("Edge Mid Pos: (%d, %d, %d)", edge_info.pos.x, edge_info.pos.y, edge_info.pos.z);

						if (ImGui::Button("Go")) {
							// 设置相机位置的事件

							EventSystem::SetCameraPosEvent e{ edge_info.pos };
							EventSystem::Dispatcher::GetInstance().Dispatch(e);

						}

						ImGui::TreePop();
					}

					if (color_flag) {
						ImGui::PopStyleColor();
					}
					ImGui::PopID();
				}


				ImGui::TreePop();
			}
		}

		// TODO
		void RenderHalfEdgeInfos() {

		}

		// TODO
		void RenderLoopInfos() {
		}

		// TODO
		void RenderFaceInfos() {
		}


		void Render(
			const RenderInfo& renderInfo
		) override {

			ImGui::Begin("SAT Info");

			RenderVertexInfos();
			RenderEdgeInfos();
			RenderHalfEdgeInfos();
			RenderLoopInfos();
			RenderFaceInfos();

			ImGui::End();
		}

		SatGuiRenderer(Info::SatInfo& satInfo) : satInfo(satInfo) {}

		~SatGuiRenderer() {}

	};

}