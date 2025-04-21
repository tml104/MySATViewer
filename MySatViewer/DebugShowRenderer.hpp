#pragma once


#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "RenderInfo.hpp"
#include "IRenderable.hpp"

#include "shader_s.h"

#include "DebugShowInfo.hpp"

namespace MyRenderEngine {

	class DebugShowRenderer : public IRenderable {
	public:
		unsigned int VAO;
		unsigned int VBO;

		// points
		std::vector<float> points_with_color; // 6x size of point

		Shader* shader;

		glm::mat4 modelMatrix{ 1.0f };


		void LoadFromDebugShowInfo(const Info::DebugShowInfo& info) {
			points_with_color.clear();

			for (auto&& pointInfo : info.things.pointInfos) {
				points_with_color.emplace_back(pointInfo.pos.x);
				points_with_color.emplace_back(pointInfo.pos.y);
				points_with_color.emplace_back(pointInfo.pos.z);
				points_with_color.emplace_back(pointInfo.color.x);
				points_with_color.emplace_back(pointInfo.color.y);
				points_with_color.emplace_back(pointInfo.color.z);
			}

			glGenBuffers(1, &VBO);
			glGenVertexArrays(1, &VAO);

			glBindVertexArray(VAO);
			glBindBuffer(GL_ARRAY_BUFFER, VBO);
			glBufferData(GL_ARRAY_BUFFER, sizeof(float) * points_with_color.size(), points_with_color.data(), GL_STATIC_DRAW);

			// ÊôÐÔ£ºpos
			glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
			glEnableVertexAttribArray(0);

			// ÊôÐÔ£ºcolor
			glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
			glEnableVertexAttribArray(1);

			glBindVertexArray(0);

		}

		void Render(
			const RenderInfo& renderInfo
		) override {
			shader->use();

			shader->setMatrix4("projection", renderInfo.projectionMatrix);
			shader->setMatrix4("view", renderInfo.viewMatrix);

			shader->setMatrix4("model", glm::scale(modelMatrix, glm::vec3(renderInfo.scaleFactor)));

			glBindVertexArray(VAO);
			glDrawArrays(GL_POINTS, 0, static_cast<GLsizei>(points_with_color.size() / 6));
			glBindVertexArray(0);
		}

		DebugShowRenderer(Shader* shader) : shader(shader) {
			
		}

		~DebugShowRenderer() {
			glDeleteVertexArrays(1, &VAO);
			glDeleteBuffers(1, &VBO);
		}
	};

}