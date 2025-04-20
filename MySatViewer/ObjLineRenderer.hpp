#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "RenderInfo.hpp"
#include "IRenderable.hpp"

#include "shader_s.h"

#include "ObjInfo.hpp"
#include "ObjMarkNum.hpp"


namespace MyRenderEngine {

class ObjLineRenderer : public IRenderable {
public:

	std::vector<float> yellow_lines;
	std::vector<float> green_lines;
	std::vector<float> red_lines;

	unsigned int yellowVAO;
	unsigned int yellowVBO;

	unsigned int greenVAO;
	unsigned int greenVBO;

	unsigned int redVAO;
	unsigned int redVBO;

	Shader* shader;

	glm::mat4 modelMatrix{ 1.0f };

	void _DeleteBuffers() {
		glDeleteVertexArrays(1, &yellowVAO);
		glDeleteVertexArrays(1, &greenVAO);
		glDeleteVertexArrays(1, &redVAO);
		glDeleteBuffers(1, &yellowVBO);
		glDeleteBuffers(1, &greenVBO);
		glDeleteBuffers(1, &redVBO);
	}

	void SetUp() {

		yellow_lines.clear();
		green_lines.clear();
		red_lines.clear();

		auto& objMarkNum = ObjMarkNum::GetInstance();

		// 添加顶点坐标到3个vector中，每个vector中按照3个开始点，3个结束点这样交替存放顶点
		for (auto e_pair : objMarkNum.edgesMap) {
			auto e = e_pair.second;

			auto st_coord = e->st->pointCoord;
			auto ed_coord = e->ed->pointCoord;
			if (e->halfEdges.size() == 2) { // green
				green_lines.emplace_back(st_coord.x());
				green_lines.emplace_back(st_coord.y());
				green_lines.emplace_back(st_coord.z());

				green_lines.emplace_back(ed_coord.x());
				green_lines.emplace_back(ed_coord.y());
				green_lines.emplace_back(ed_coord.z());
			}
			else if (e->halfEdges.size() == 1) { // red
				red_lines.emplace_back(st_coord.x());
				red_lines.emplace_back(st_coord.y());
				red_lines.emplace_back(st_coord.z());

				red_lines.emplace_back(ed_coord.x());
				red_lines.emplace_back(ed_coord.y());
				red_lines.emplace_back(ed_coord.z());
			}
			else { //yellow
				yellow_lines.emplace_back(st_coord.x());
				yellow_lines.emplace_back(st_coord.y());
				yellow_lines.emplace_back(st_coord.z());

				yellow_lines.emplace_back(ed_coord.x());
				yellow_lines.emplace_back(ed_coord.y());
				yellow_lines.emplace_back(ed_coord.z());
			}
		}

		auto set_vao = [&](unsigned int& VAO, unsigned int& VBO, std::vector<float>& lines) {
			glGenVertexArrays(1, &VAO);
			glGenBuffers(1, &VBO);
			//glGenBuffers(1, &EBO);

			glBindVertexArray(VAO);
			glBindBuffer(GL_ARRAY_BUFFER, VBO);
			glBufferData(GL_ARRAY_BUFFER, sizeof(float) * lines.size(), lines.data(), GL_STATIC_DRAW);

			//glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
			//glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(unsigned int) * objInfo.indices.size(), objInfo.indices.data(), GL_STATIC_DRAW);

			glEnableVertexAttribArray(0);
			glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
			//glEnableVertexAttribArray(1);
			//glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));

			glBindVertexArray(0);
			};


		_DeleteBuffers();

		set_vao(yellowVAO, yellowVBO, yellow_lines);
		set_vao(redVAO, redVBO, red_lines);
		set_vao(greenVAO, greenVBO, green_lines);
	}

	void Render(
		const RenderInfo& renderInfo
	) override {
		shader->use();

		shader->setMatrix4("projection", renderInfo.projectionMatrix);
		shader->setMatrix4("view", renderInfo.viewMatrix);

		shader->setMatrix4("model", glm::scale(modelMatrix, glm::vec3(renderInfo.scaleFactor)));


		shader->setVec3("subcolor", glm::vec3(1.0f, 1.0f, 0.0f));
		glBindVertexArray(yellowVAO);
		glDrawArrays(GL_LINES, 0, yellow_lines.size() / 3);
		glBindVertexArray(0);

		shader->setVec3("subcolor", glm::vec3(1.0f, 0.0f, 0.0f));
		glBindVertexArray(redVAO);
		glDrawArrays(GL_LINES, 0, red_lines.size() / 3);
		glBindVertexArray(0);

		shader->setVec3("subcolor", glm::vec3(0.0f, 1.0f, 0.0f));
		glBindVertexArray(greenVAO);
		glDrawArrays(GL_LINES, 0, green_lines.size() / 3);
		glBindVertexArray(0);
	}

	ObjLineRenderer(Shader* shader) : shader(shader) {
		SetUp();
	}

	~ObjLineRenderer() {
		_DeleteBuffers();
	}
};

}