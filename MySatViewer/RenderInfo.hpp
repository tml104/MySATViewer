#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

namespace MyRenderEngine {

	/*
		��������Ⱦѭ���������IRenderable������Ⱦ����Ϣ
	*/
	struct RenderInfo {
		glm::mat4 projectionMatrix;
		glm::mat4 viewMatrix;
		glm::vec3 cameraPos;
		float scaleFactor;

		bool showModel;
		bool transparentModel; // ��͸��������Ҫͨ����������жϵ����ĸ���ɫ������Ȼ��Ⱦ����Target������Ҫ��ǰ�ֶ�ָ����

		RenderInfo() :
			showModel(false),
			transparentModel(false)
		{
		}
	};
}