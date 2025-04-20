#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "RenderInfo.hpp"
#include "IRenderable.hpp"

#include "shader_s.h"

#include "ObjInfo.hpp"
#include "ObjMarkNum.hpp"

#include "SetCameraPosEvent.hpp"
#include "Dispatcher.hpp"

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

namespace MyRenderEngine {

	class ObjGuiRenderer: public IRenderable {
	public:

		struct YellowInfo {
			int halfEdgesCount;
			int edgeMarkNum;

			int stMarkNum;
			int edMarkNum;
		};

		std::vector<YellowInfo> yellowInfos;
		std::vector<YellowInfo> greenInfos;
		std::vector<YellowInfo> redInfos;

		// TODO: need to improve design here
		glm::mat4 modelMatrix{ 1.0f };


		void SetUp() {
			yellowInfos.clear();
			greenInfos.clear();
			redInfos.clear();

			auto& objMarkNum = ObjMarkNum::GetInstance();

			for (auto e_pair : objMarkNum.edgesMap) {

				auto e = e_pair.second;

				std::vector<YellowInfo>* infos_ptr = nullptr;

				if (e->halfEdges.size() == 2) { // green
					infos_ptr = &greenInfos;
				}
				else if (e->halfEdges.size() == 1) { // red
					infos_ptr = &redInfos;
				}
				else { //yellow
					infos_ptr = &yellowInfos;
				}

				if (infos_ptr) {
					infos_ptr->push_back({ static_cast<int>(e->halfEdges.size()), objMarkNum.GetId(e), objMarkNum.GetId(e->st), objMarkNum.GetId(e->ed) });
				}
			}
		}

		void RenderGui(const RenderInfo& renderInfo) {

			auto tree_node_render = [&](const std::string& name, const std::vector<YellowInfo>& infos) {
					
				const std::string edge_type_name = name + " Edges";
				if (ImGui::TreeNode(edge_type_name.c_str())) {

					int id = 0;
					for (int i = 0; i < infos.size(); i++) {
						ImGui::PushID(id);
						if (ImGui::TreeNode("", "%s: %d", name.c_str(), i)) {
							ImGui::Text("half edges count: %d", infos[id].halfEdgesCount);
							ImGui::Text("edge MarkNum: %d", infos[id].edgeMarkNum);
							ImGui::Text("st MarkNum: %d", infos[id].stMarkNum);
							ImGui::Text("ed MarkNum: %d", infos[id].edMarkNum);

							if (ImGui::Button("Go")) {
								// 设置相机位置的事件

								auto& objMarkNum = ObjMarkNum::GetInstance();
								Topology::Vertex* st_vertex_ptr = static_cast<Topology::Vertex*>(objMarkNum.GetEntityPtr({ TopoType::Vertex, infos[id].stMarkNum }));
								Topology::Vertex* ed_vertex_ptr = static_cast<Topology::Vertex*>(objMarkNum.GetEntityPtr({ TopoType::Vertex, infos[id].edMarkNum }));

								Coordinate pos = (st_vertex_ptr->pointCoord + ed_vertex_ptr->pointCoord) / 2.0f;
								glm::vec3 pos_in_glm{ pos.x(), pos.y(), pos.z() };

								// 考虑model矩阵和renderInfo.scaleFactor的变换
								pos_in_glm = glm::vec3(glm::vec4(pos_in_glm, 1.0f) * modelMatrix * renderInfo.scaleFactor);

								EventSystem::SetCameraPosEvent e{pos_in_glm };
								auto& dispatcher = EventSystem::Dispatcher::GetInstance();
								dispatcher.Dispatch(e);
							}
							ImGui::TreePop();
						}

						ImGui::PopID();
						id++;
					}

					ImGui::TreePop();
				}

			};


			ImGui::Begin("OBJ Edges Info");

			tree_node_render("Red", redInfos);
			tree_node_render("Yellow", yellowInfos);
			tree_node_render("Green", greenInfos);

			ImGui::End();
		}

		void Render(
			const RenderInfo& renderInfo
		) override {

			RenderGui(renderInfo);

		}

	};

}