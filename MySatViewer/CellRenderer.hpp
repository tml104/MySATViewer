#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "RenderInfo.hpp"
#include "IRenderable.hpp"

#include "shader_s.h"

#include "SatInfo.hpp"

#include "CellInfo.hpp"

namespace MyRenderEngine {

	class CellRenderer : public IRenderable {
	public:
		std::vector<float> lines;

		unsigned int lineVAO;
		unsigned int lineVBO;
		
		Shader* shader;

		glm::vec3 color{0.0f, 0.0f, 1.0f}; // Blue {0.0f, 0.0f, 1.0f}  or Yellow {1.0f, 1.0f, 0.0f}
		glm::mat4 modelMatrix{ 1.0f };


		void _DeleteBuffers() {
			glDeleteVertexArrays(1, &lineVAO);
			glDeleteBuffers(1, &lineVBO);
		}

		void Render(const RenderInfo& renderInfo) override {
			shader->use();

			shader->setMatrix4("projection", renderInfo.projectionMatrix);
			shader->setMatrix4("view", renderInfo.viewMatrix);
			shader->setMatrix4("model", glm::scale(modelMatrix, glm::vec3(renderInfo.scaleFactor)));


			shader->setVec3("subcolor", color);
			glBindVertexArray(lineVAO);
			glDrawArrays(GL_LINES, 0, lines.size() / 3); // 最后一个参数是顶点个数（而非图元个数）
			glBindVertexArray(0);
		}

		void LoadFromCellInfo(const Info::CellInfo& cellInfo, glm::vec3 new_color ) {

			lines.clear();
			_DeleteBuffers();

			color = new_color;

			for (auto&& one_box_info : cellInfo.things.cellBoxInfos) {

				// 12 条线

				// l1
				lines.emplace_back(one_box_info.min_point[0]);
				lines.emplace_back(one_box_info.min_point[1]);
				lines.emplace_back(one_box_info.min_point[2]);

				lines.emplace_back(one_box_info.max_point[0]);
				lines.emplace_back(one_box_info.min_point[1]);
				lines.emplace_back(one_box_info.min_point[2]);

				// l2
				lines.emplace_back(one_box_info.max_point[0]);
				lines.emplace_back(one_box_info.min_point[1]);
				lines.emplace_back(one_box_info.min_point[2]);

				lines.emplace_back(one_box_info.max_point[0]);
				lines.emplace_back(one_box_info.max_point[1]);
				lines.emplace_back(one_box_info.min_point[2]);

				// l3
				lines.emplace_back(one_box_info.max_point[0]);
				lines.emplace_back(one_box_info.max_point[1]);
				lines.emplace_back(one_box_info.min_point[2]);

				lines.emplace_back(one_box_info.min_point[0]);
				lines.emplace_back(one_box_info.max_point[1]);
				lines.emplace_back(one_box_info.min_point[2]);

				// l4
				lines.emplace_back(one_box_info.min_point[0]);
				lines.emplace_back(one_box_info.max_point[1]);
				lines.emplace_back(one_box_info.min_point[2]);

				lines.emplace_back(one_box_info.min_point[0]);
				lines.emplace_back(one_box_info.min_point[1]);
				lines.emplace_back(one_box_info.min_point[2]);

				// l5
				lines.emplace_back(one_box_info.min_point[0]);
				lines.emplace_back(one_box_info.min_point[1]);
				lines.emplace_back(one_box_info.min_point[2]);

				lines.emplace_back(one_box_info.min_point[0]);
				lines.emplace_back(one_box_info.min_point[1]);
				lines.emplace_back(one_box_info.max_point[2]);

				// l6
				lines.emplace_back(one_box_info.max_point[0]);
				lines.emplace_back(one_box_info.min_point[1]);
				lines.emplace_back(one_box_info.min_point[2]);

				lines.emplace_back(one_box_info.max_point[0]);
				lines.emplace_back(one_box_info.min_point[1]);
				lines.emplace_back(one_box_info.max_point[2]);

				// l7
				lines.emplace_back(one_box_info.max_point[0]);
				lines.emplace_back(one_box_info.max_point[1]);
				lines.emplace_back(one_box_info.min_point[2]);

				lines.emplace_back(one_box_info.max_point[0]);
				lines.emplace_back(one_box_info.max_point[1]);
				lines.emplace_back(one_box_info.max_point[2]);

				// l8
				lines.emplace_back(one_box_info.min_point[0]);
				lines.emplace_back(one_box_info.max_point[1]);
				lines.emplace_back(one_box_info.min_point[2]);

				lines.emplace_back(one_box_info.min_point[0]);
				lines.emplace_back(one_box_info.max_point[1]);
				lines.emplace_back(one_box_info.max_point[2]);

				
				// l9
				lines.emplace_back(one_box_info.min_point[0]);
				lines.emplace_back(one_box_info.min_point[1]);
				lines.emplace_back(one_box_info.max_point[2]);

				lines.emplace_back(one_box_info.max_point[0]);
				lines.emplace_back(one_box_info.min_point[1]);
				lines.emplace_back(one_box_info.max_point[2]);

				// l10
				lines.emplace_back(one_box_info.max_point[0]);
				lines.emplace_back(one_box_info.min_point[1]);
				lines.emplace_back(one_box_info.max_point[2]);

				lines.emplace_back(one_box_info.max_point[0]);
				lines.emplace_back(one_box_info.max_point[1]);
				lines.emplace_back(one_box_info.max_point[2]);

				// l11
				lines.emplace_back(one_box_info.max_point[0]);
				lines.emplace_back(one_box_info.max_point[1]);
				lines.emplace_back(one_box_info.max_point[2]);

				lines.emplace_back(one_box_info.min_point[0]);
				lines.emplace_back(one_box_info.max_point[1]);
				lines.emplace_back(one_box_info.max_point[2]);

				// l12
				lines.emplace_back(one_box_info.min_point[0]);
				lines.emplace_back(one_box_info.max_point[1]);
				lines.emplace_back(one_box_info.max_point[2]);

				lines.emplace_back(one_box_info.min_point[0]);
				lines.emplace_back(one_box_info.min_point[1]);
				lines.emplace_back(one_box_info.max_point[2]);
			}

			glGenVertexArrays(1, &lineVAO);
			glGenBuffers(1, &lineVBO);

			glBindVertexArray(lineVAO);
			glBindBuffer(GL_ARRAY_BUFFER, lineVBO);
			glBufferData(GL_ARRAY_BUFFER, sizeof(float) * lines.size(), lines.data(), GL_STATIC_DRAW);

			glEnableVertexAttribArray(0);
			glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);

			glBindVertexArray(0);
		}

		CellRenderer(Shader* shader): shader(shader){}

		~CellRenderer() {

			_DeleteBuffers();

		}

	};


}