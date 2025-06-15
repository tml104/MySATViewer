#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "RenderInfo.hpp"
#include "IRenderable.hpp"

#include "shader_s.h"

#include "SatInfo.hpp"

#include "RayInfo.hpp"

namespace MyRenderEngine {


	class RayRenderer : public IRenderable {

	public:
		std::vector<float> green_lines;
		std::vector<float> purple_lines;

		unsigned int greenVAO;
		unsigned int greenVBO;
		unsigned int purpleVAO;
		unsigned int purpleVBO;

		Shader* shader;

		glm::mat4 modelMatrix{ 1.0f };

		const glm::vec3 green_color{ 0.0f, 1.0f, 0.0f };
		const glm::vec3 purple_color{1.0f, 0.0f, 1.0f };
		
		void _DeleteBuffers() {

			glDeleteVertexArrays(1, &greenVAO);
			glDeleteVertexArrays(1, &purpleVAO);
			glDeleteBuffers(1, &greenVBO);
			glDeleteBuffers(1, &purpleVBO);

		}

		void LoadFromRayInfo(const Info::RayInfo& rayInfo) {

			for (auto&& one_ray_info : rayInfo.things.rayInfos) {

				glm::vec3 ed = one_ray_info.start_point + one_ray_info.direction * one_ray_info.r;

				
				// 这里只能用指针，不能用引用，因为引用不能修改所引用的对象
				auto target_lines_ptr = &green_lines;

				if (one_ray_info.result == 0) {
					target_lines_ptr = &green_lines;
				}
				else {
					target_lines_ptr = &purple_lines;
				}

				target_lines_ptr->emplace_back(one_ray_info.start_point.x);
				target_lines_ptr->emplace_back(one_ray_info.start_point.y);
				target_lines_ptr->emplace_back(one_ray_info.start_point.z);

				target_lines_ptr->emplace_back(ed.x);
				target_lines_ptr->emplace_back(ed.y);
				target_lines_ptr->emplace_back(ed.z);

			}

			glGenVertexArrays(1, &greenVAO);
			glGenBuffers(1, &greenVBO);

			glBindVertexArray(greenVAO);
			glBindBuffer(GL_ARRAY_BUFFER, greenVBO);
			glBufferData(GL_ARRAY_BUFFER, sizeof(float) * green_lines.size(), green_lines.data(), GL_STATIC_DRAW);

			glEnableVertexAttribArray(0);
			glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);

			glBindVertexArray(0);

			glGenVertexArrays(1, &purpleVAO);
			glGenBuffers(1, &purpleVBO);

			glBindVertexArray(purpleVAO);
			glBindBuffer(GL_ARRAY_BUFFER, purpleVBO);
			glBufferData(GL_ARRAY_BUFFER, sizeof(float) * purple_lines.size(), purple_lines.data(), GL_STATIC_DRAW);

			glEnableVertexAttribArray(0);
			glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);

			glBindVertexArray(0);
		}


		void Render(
			const RenderInfo& renderInfo
		) {
			shader->use();

			shader->setMatrix4("projection", renderInfo.projectionMatrix);
			shader->setMatrix4("view", renderInfo.viewMatrix);
			shader->setMatrix4("model", glm::scale(modelMatrix, glm::vec3(renderInfo.scaleFactor)));


			shader->setVec3("subcolor", green_color);
			glBindVertexArray(greenVAO);
			glDrawArrays(GL_LINES, 0, green_lines.size() / 3);
			glBindVertexArray(0);

			shader->setVec3("subcolor", purple_color);
			glBindVertexArray(purpleVAO);
			glDrawArrays(GL_LINES, 0, purple_lines.size() / 3);
			glBindVertexArray(0);
		}

		RayRenderer(Shader* shader) : shader(shader) {


		}

		~RayRenderer() {
			_DeleteBuffers();
		}
	};

}