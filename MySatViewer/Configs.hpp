#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>


namespace MyRenderEngine {
	// Configs
	namespace Configs {
		const glm::vec4 GREEN_BACKGROUND{ 0.2f, 0.3f, 0.3f, 1.0f };
		const glm::vec4 BLACK_BACKGROUND{ 0.0f, 0.0f, 0.0f, 0.0f };

		const glm::vec3 CAMERA_INIT_POS{ 0.0f, 0.0f, 5.0f };

		const unsigned int SCR_WIDTH = 1920;
		const unsigned int SCR_HEIGHT = 1080;
		const unsigned int SCR_X_POS = 200;
		const unsigned int SCR_Y_POS = 200;

		const glm::vec4 ZERO_VEC(0.0f);
		const glm::vec4 ONE_VEC(1.0f);
	}
}
