#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "RenderInfo.hpp"
#include "IRenderable.hpp"

#include "MyRenderEngine.hpp"

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

namespace MyRenderEngine {

	class BasicGuiRenderer : public IRenderable {
	public:

		void Render(
			const RenderInfo& renderInfo
		) override {
			ImGui::Begin("Basic Info");

			// ���λ��
			float& camX = myRenderEngine.camera.Position.x;
			float& camY = myRenderEngine.camera.Position.y;
			float& camZ = myRenderEngine.camera.Position.z;

			ImGui::InputFloat("Camera X", &camX);
			ImGui::InputFloat("Camera Y", &camY);
			ImGui::InputFloat("Camera Z", &camZ);

			// �ƶ��ٶ�
			ImGui::InputFloat("Camera Speed", &myRenderEngine.camera.MovementSpeed);

			// ģ�ͷŴ���
			ImGui::InputFloat("Scale Factor", &myRenderEngine.scaleFactor, 0.1f, 1.0f, "%.3f");

			// ��ʾģ��
			ImGui::Checkbox("Show Model", &myRenderEngine.showModel);

			// ģ��͸��
			ImGui::Checkbox("Transparent Model", &myRenderEngine.transparentModel);

			ImGui::End();
		}

		// TODO: Ҫ���������������
		BasicGuiRenderer(MyRenderEngine& myRenderEngine): myRenderEngine(myRenderEngine) {

		}

	private:
		MyRenderEngine& myRenderEngine;
	};

}