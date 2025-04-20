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

			// 相机位置
			float& camX = myRenderEngine.camera.Position.x;
			float& camY = myRenderEngine.camera.Position.y;
			float& camZ = myRenderEngine.camera.Position.z;

			ImGui::InputFloat("Camera X", &camX);
			ImGui::InputFloat("Camera Y", &camY);
			ImGui::InputFloat("Camera Z", &camZ);

			// 移动速度
			ImGui::InputFloat("Camera Speed", &myRenderEngine.camera.MovementSpeed);

			// 模型放大倍率
			ImGui::InputFloat("Scale Factor", &myRenderEngine.scaleFactor, 0.1f, 1.0f, "%.3f");

			// 显示模型
			ImGui::Checkbox("Show Model", &myRenderEngine.showModel);

			// 模型透明
			ImGui::Checkbox("Transparent Model", &myRenderEngine.transparentModel);

			ImGui::End();
		}

		// TODO: 要尽量避免这种情况
		BasicGuiRenderer(MyRenderEngine& myRenderEngine): myRenderEngine(myRenderEngine) {

		}

	private:
		MyRenderEngine& myRenderEngine;
	};

}