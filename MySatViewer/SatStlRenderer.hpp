#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "RenderInfo.hpp"
#include "IRenderable.hpp"

#include "shader_s.h"

#include "SatInfo.hpp"

namespace MyRenderEngine {

	class SatStlRenderer : public IRenderable {
	public:
		unsigned int VAO;
		unsigned int VBO;
		int stlVerticesCount;

		Shader* shader;
		Shader* transparentShader;

		glm::mat4 modelMatrix{ 1.0f };

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
				s->setMatrix4("model", modelMatrix);
				s->setVec3("viewPos", renderInfo.cameraPos);

				glBindVertexArray(VAO);
				glDrawArrays(GL_TRIANGLES, 0, stlVerticesCount);
			}
		}

		void LoadFromSatInfo(Info::SatInfo& satInfo) {
			stlVerticesCount = satInfo.stl.stlVerticesCount;

			glGenVertexArrays(1, &VAO);
			glGenBuffers(1, &VBO);

			glBindVertexArray(VAO);
			glBindBuffer(GL_ARRAY_BUFFER, VBO);
			glBufferData(GL_ARRAY_BUFFER, sizeof(float) * satInfo.stl.stlVertices.size(), satInfo.stl.stlVertices.data(), GL_STATIC_DRAW);

			glEnableVertexAttribArray(0);
			glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
			glEnableVertexAttribArray(1);
			glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));

			glBindBuffer(GL_ARRAY_BUFFER, 0);
			glBindVertexArray(0);
		}

		SatStlRenderer(Shader* shader, Shader* transparentShader) :
			shader(shader),
			transparentShader(transparentShader),
			VAO(0),
			VBO(0)
		{
		}

		~SatStlRenderer() {}
	};
}