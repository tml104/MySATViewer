#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

namespace MyRenderEngine {

	/*
		用来在渲染循环中向各个IRenderable传递渲染用信息
	*/
	struct RenderInfo {
		glm::mat4 projectionMatrix;
		glm::mat4 viewMatrix;
		glm::vec3 cameraPos;
		float scaleFactor;

		bool showModel;
		bool transparentModel; // 可透明物体需要通过这个变量判断调用哪个着色器，当然渲染到的Target还是需要提前手动指定的

		RenderInfo() :
			showModel(false),
			transparentModel(false)
		{
		}
	};
}