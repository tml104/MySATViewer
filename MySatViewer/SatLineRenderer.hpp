#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "RenderInfo.hpp"
#include "IRenderable.hpp"

#include "shader_s.h"

#include "SatInfo.hpp"

namespace MyRenderEngine {

	class SatLineRenderer : public IRenderable {
	public:
		std::vector<unsigned int> VAOs;
		std::vector<unsigned int> VBOs;
		std::vector<int> edgeSampledPointsCounts;
		std::vector<glm::vec3> edgeColors;
		Shader* shader;

		glm::mat4 modelMatrix{ 1.0f };

		void Render(
			const RenderInfo& renderInfo
		) override {
			shader->use();

			shader->setMatrix4("projection", renderInfo.projectionMatrix);
			shader->setMatrix4("view", renderInfo.viewMatrix);
			shader->setMatrix4("model", modelMatrix);
			//shader.setVec3("viewPos", camera_pos);

			// 逐个绘制线
			for (int h = 0; h < VAOs.size(); h++) {
				int VAO = VAOs[h];
				int VAO_size = edgeSampledPointsCounts[h];
				glm::vec3 color = edgeColors[h];

				shader->setVec3("subcolor", color);
				glBindVertexArray(VAO);
				glDrawArrays(GL_LINE_STRIP, 0, VAO_size);
			}
		}

		void LoadFromSatInfo(Info::SatInfo& satInfo) {
			VAOs.clear();
			VBOs.clear();

			edgeSampledPointsCounts.clear();
			edgeColors.clear();


			for (Info::EdgeInfo& edgeInfo : satInfo.brepInfo.edgeInfos) {
				auto& geometryPtr = edgeInfo.geometryPtr;

				edgeSampledPointsCounts.emplace_back(geometryPtr->sampledPoints.size());
				edgeColors.emplace_back(edgeInfo.GetColor());

				// 数据扁平化，使得其能输入到openGL中
				std::vector<float> sampledPointsDataAsArray;

				for (glm::vec3& point : geometryPtr->sampledPoints) {
					sampledPointsDataAsArray.emplace_back(point.x);
					sampledPointsDataAsArray.emplace_back(point.y);
					sampledPointsDataAsArray.emplace_back(point.z);
				}

				unsigned VAO, VBO;
				glGenVertexArrays(1, &VAO);
				glGenBuffers(1, &VBO);

				glBindVertexArray(VAO);
				glBindBuffer(GL_ARRAY_BUFFER, VBO);
				glBufferData(GL_ARRAY_BUFFER, sizeof(float) * sampledPointsDataAsArray.size(), sampledPointsDataAsArray.data(), GL_STATIC_DRAW);

				glEnableVertexAttribArray(0);
				glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
				glBindBuffer(GL_ARRAY_BUFFER, 0);
				glBindVertexArray(0);

				VAOs.emplace_back(VAO);
				VBOs.emplace_back(VBO);
			}


			//edgeSampledPointsCounts = satInfo.edgeSampledPointsCounts;
			//edgeColors = satInfo.edgeColors;

			//for (auto vertices : satInfo.edgeSampledPoints) {
			//	unsigned VAO, VBO;
			//	glGenVertexArrays(1, &VAO);
			//	glGenBuffers(1, &VBO);

			//	glBindVertexArray(VAO);
			//	glBindBuffer(GL_ARRAY_BUFFER, VBO);
			//	glBufferData(GL_ARRAY_BUFFER, sizeof(float) * vertices.size(), vertices.data(), GL_STATIC_DRAW);

			//	glEnableVertexAttribArray(0);
			//	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);

			//	glBindBuffer(GL_ARRAY_BUFFER, 0);
			//	glBindVertexArray(0);

			//	VAOs.emplace_back(VAO);
			//	VBOs.emplace_back(VBO);
			//}
		}

		SatLineRenderer(Shader* shader) :
			shader(shader)
		{
		}

		~SatLineRenderer() {
			for (int i = 0; i < VAOs.size(); i++) {
				glDeleteVertexArrays(1, &VAOs[i]);
				glDeleteBuffers(1, &VBOs[i]);
			}
			VAOs.clear();
			VBOs.clear();
		}
	};

}