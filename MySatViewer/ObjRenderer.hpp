#pragma once

#include "shader_s.h"

#include "RenderInfo.hpp"
#include "IRenderable.hpp"

#include "ObjInfo.hpp"
#include "TopologyInfo.hpp"


namespace MyRenderEngine {

	class ObjRenderer : public IRenderable {
	public:
		Info::ObjInfo& objInfo;

		unsigned int VAO;
		unsigned int VBO;

		std::vector<float> newVerticesWithNormal;
		int verticesCount;

		Shader* shader;
		Shader* transparentShader;

		glm::mat4 modelMatrix{ 1.0f };

		void Setup() {
			newVerticesWithNormal.clear();
			verticesCount = 0;
			VAO = VBO = 0;

			for (int i = 0; i < objInfo.indices.size(); i += 3) {
				int j = i + 1;
				int k = i + 2;

				int index_i = objInfo.indices[i];
				int index_j = objInfo.indices[j];
				int index_k = objInfo.indices[k];

				auto p1 = objInfo.GetPoint(index_i);
				auto p2 = objInfo.GetPoint(index_j);
				auto p3 = objInfo.GetPoint(index_k);

				auto v12 = p2 - p1;
				auto v13 = p3 - p1;
				auto normal = v12.Cross(v13);

				newVerticesWithNormal.emplace_back(p1.x());
				newVerticesWithNormal.emplace_back(p1.y());
				newVerticesWithNormal.emplace_back(p1.z());
				newVerticesWithNormal.emplace_back(normal.x());
				newVerticesWithNormal.emplace_back(normal.y());
				newVerticesWithNormal.emplace_back(normal.z());

				newVerticesWithNormal.emplace_back(p2.x());
				newVerticesWithNormal.emplace_back(p2.y());
				newVerticesWithNormal.emplace_back(p2.z());
				newVerticesWithNormal.emplace_back(normal.x());
				newVerticesWithNormal.emplace_back(normal.y());
				newVerticesWithNormal.emplace_back(normal.z());

				newVerticesWithNormal.emplace_back(p3.x());
				newVerticesWithNormal.emplace_back(p3.y());
				newVerticesWithNormal.emplace_back(p3.z());
				newVerticesWithNormal.emplace_back(normal.x());
				newVerticesWithNormal.emplace_back(normal.y());
				newVerticesWithNormal.emplace_back(normal.z());

				verticesCount += 3;
			}

			glGenVertexArrays(1, &VAO);
			glGenBuffers(1, &VBO);

			glBindVertexArray(VAO);
			glBindBuffer(GL_ARRAY_BUFFER, VBO);
			glBufferData(GL_ARRAY_BUFFER, sizeof(float) * newVerticesWithNormal.size(), newVerticesWithNormal.data(), GL_STATIC_DRAW);

			glEnableVertexAttribArray(0);
			glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
			glEnableVertexAttribArray(1);
			glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));

			glBindBuffer(GL_ARRAY_BUFFER, 0);
			glBindVertexArray(0);
		}

		void Render(
			const RenderInfo& renderInfo
		) override {
			if (renderInfo.showModel) {
				Shader* s;

				if (renderInfo.transparentModel == false) {
					s = shader;
				}
				else {
					s = transparentShader;
				}

				s->use();

				s->setMatrix4("projection", renderInfo.projectionMatrix);
				s->setMatrix4("view", renderInfo.viewMatrix);
				s->setMatrix4("model", glm::scale(modelMatrix, glm::vec3(renderInfo.scaleFactor)));
				s->setVec3("viewPos", renderInfo.cameraPos);

				glBindVertexArray(VAO);
				//glDrawElements(GL_TRIANGLES, static_cast<unsigned int>(objInfo.indices.size()), GL_UNSIGNED_INT, 0);
				glDrawArrays(GL_TRIANGLES, 0, verticesCount);
				glBindVertexArray(0);

			}
		}

		ObjRenderer(Info::ObjInfo& objInfo, Shader* shader, Shader* transparentShader) : objInfo(objInfo), shader(shader), transparentShader(transparentShader), VAO(0), VBO(0), verticesCount(0) {}
		~ObjRenderer() {}
	};

}