#pragma once

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <iostream>
#include <fstream>
#include <memory>

#include "shader_s.h"
#include "camera.h"

#include "model.h"
#include "mesh.h"

#include "stl_reader.h"
#include "json.hpp"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

using json = nlohmann::json;

namespace MyRenderEngine {

	struct EdgeInfo {
		int edgeIndex;
		int markNum;
		int bodyId;
		int nonmanifoldCount;

		glm::vec3 edgeMidPos;
	};

	class SatInfo {
	public:
		std::vector<float> stlVertices; // ע�⣺��ÿ������˵������6��float����Ϣ������3����normal 3���������ʵ�ʲ�����������Ҫ����6
		int stlVerticesCount; // ��¼���ǲ������غ������stl�����ж�����

		std::vector<std::vector<float>> edgeSampledPoints; // ע�⣺����һ��edge���ɶ����߶���ɵģ�ÿ��vector�����ÿ������˵������3��float����Ϣ�����ʵ�ʲ�����������Ҫ����3
		std::vector<int> edgeSampledPointsCounts;
		std::vector<glm::vec3> edgeColors;
		std::unordered_map<int, EdgeInfo> edgeMap; // ÿ���ߵ� Marknum -> edgeSampledPoints��vector�Ķ�Ӧ����

		glm::vec3 newCameraPos;

		void LoadStl(const std::string& stl_path) {
			stlVertices.clear();
			stlVerticesCount = 0;
			newCameraPos = glm::vec3{ 100.0f };

			stl_reader::StlMesh <float, unsigned int> mesh(stl_path);

			for (size_t i_solid = 0; i_solid < mesh.num_solids(); i_solid++) {
				for (size_t j_tri = mesh.solid_tris_begin(i_solid); j_tri < mesh.solid_tris_end(i_solid); j_tri++) {

					glm::vec3 triangle_points[3];

					for (int j = 0; j < 3; j++) {
						triangle_points[j].x = mesh.tri_corner_coords(j_tri, j)[0];
						triangle_points[j].y = mesh.tri_corner_coords(j_tri, j)[1];
						triangle_points[j].z = mesh.tri_corner_coords(j_tri, j)[2];
					}

					glm::vec3 u = triangle_points[2] - triangle_points[0];
					glm::vec3 v = triangle_points[1] - triangle_points[0];

					glm::vec3 normalized_normal = glm::normalize(glm::cross(v, u));

					for (int j = 0; j < 3; j++) {
						stlVertices.emplace_back(triangle_points[j].x);
						stlVertices.emplace_back(triangle_points[j].y);
						stlVertices.emplace_back(triangle_points[j].z);

						stlVertices.emplace_back(normalized_normal.x);
						stlVertices.emplace_back(normalized_normal.y);
						stlVertices.emplace_back(normalized_normal.z);

						newCameraPos.x = std::min(newCameraPos.x, triangle_points[j].x);
						newCameraPos.y = std::min(newCameraPos.y, triangle_points[j].y);
						newCameraPos.z = std::min(newCameraPos.z, triangle_points[j].z);

						stlVerticesCount++;
					}
				}
			}

			newCameraPos += glm::vec3{ 20.0f }; // ƫ��
		}

		void LoadGeometryJson(const std::string& json_path, int selected_body = -1) {
			edgeSampledPoints.clear();
			edgeSampledPointsCounts.clear();
			edgeColors.clear();
			edgeMap.clear();

			std::ifstream f(json_path);
			json data = json::parse(f);

			int ii = 0;
			auto root_edges = data["root_edges"];
			for (auto&& element : root_edges) { // for each edge
				int nonmanifold_count = element["nonmanifold_count"];
				int body_id = element["body"];
				int marknum = element["marknum"];

				if (selected_body != -1 && selected_body != body_id)
				{
					continue;
				}

				if (nonmanifold_count != 2)
				{
					printf("[LoadGeometryJson] nonmanifold: edge: %d, body: %d, count: %d \n", int(element["marknum"]), body_id, nonmanifold_count);
					//spdlog::info("nonmanifold: edge: {}, body: {], count: {}", int(element["marknum"]), body_id, nonmanifold_count);
				}

				glm::vec3 color;
				if (nonmanifold_count == 1) {
					color = glm::vec3(1.0, 0.0, 0.0);
				}
				else if (nonmanifold_count == 2) {
					color = glm::vec3(0.0, 1.0, 0.0);
				}
				else if (nonmanifold_count == 4) {
					color = glm::vec3(0.0, 0.0, 1.0);
				}
				else if (nonmanifold_count == 3) {
					color = glm::vec3(1.0, 1.0, 0.0);
				}
				else {
					color = glm::vec3(0.0, 1.0, 1.0);
				}

				edgeColors.emplace_back(color);

				auto sampled_points = element["sampled_points"];
				std::vector<float> vertices;
				int point_count = 0;

				for (auto&& point : sampled_points) {
					float x = point["x"];
					float y = point["y"];
					float z = point["z"];

					//std::cout << x << " " << y << " " << z << " " << std::endl;
					vertices.emplace_back(x);
					vertices.emplace_back(y);
					vertices.emplace_back(z);

					point_count++;
				}

				edgeSampledPoints.emplace_back(vertices);
				edgeSampledPointsCounts.emplace_back(point_count);

				// mid pos
				glm::vec3 mid_pos;
				mid_pos.x = sampled_points[point_count / 2]["x"];
				mid_pos.y = sampled_points[point_count / 2]["y"];
				mid_pos.z = sampled_points[point_count / 2]["z"];

				edgeMap[marknum] = { ii++, marknum, body_id, nonmanifold_count, mid_pos };
			}
		}
	};

	struct RenderInfo {
		glm::mat4 projection_matrix;
		glm::mat4 view_matrix;
		glm::mat4 model_matrix;
		glm::vec3 camera_pos;

		bool showModel;

		RenderInfo():
			showModel(false)
		{}
	};

	class IRenderable {
	public:
		virtual void Render(
			const RenderInfo& renderInfo
		) = 0;
		virtual ~IRenderable() {};
	};

	class SatStlRenderer: public IRenderable {
	public:
		unsigned int VAO;
		unsigned int VBO;
		int stlVerticesCount;
		Shader shader;

		void Render(
			const RenderInfo& renderInfo
		) override {
			if (renderInfo.showModel) {
				shader.use();

				shader.setMatrix4("projection", renderInfo.projection_matrix);
				shader.setMatrix4("view", renderInfo.view_matrix);
				shader.setMatrix4("model", renderInfo.model_matrix);
				shader.setVec3("viewPos", renderInfo.camera_pos);

				glBindVertexArray(VAO);
				glDrawArrays(GL_TRIANGLES, 0, stlVerticesCount);
			}
		}

		void LoadFromSatInfo(SatInfo& satInfo) {
			stlVerticesCount = satInfo.stlVerticesCount;

			glGenVertexArrays(1, &VAO);
			glGenBuffers(1, &VBO);

			glBindVertexArray(VAO);
			glBindBuffer(GL_ARRAY_BUFFER, VBO);
			glBufferData(GL_ARRAY_BUFFER, sizeof(float) * satInfo.stlVertices.size(), satInfo.stlVertices.data(), GL_STATIC_DRAW);

			glEnableVertexAttribArray(0);
			glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
			glEnableVertexAttribArray(1);
			glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));

			glBindBuffer(GL_ARRAY_BUFFER, 0);
			glBindVertexArray(0);
		}

		// ͨ���ƶ�ת��shader����Ȩ
		SatStlRenderer(Shader&& shader):
			shader(std::move(shader)),
			VAO(0),
			VBO(0)
		{}

		~SatStlRenderer() {}
	};

	class SatLineRenderer : public IRenderable {
	public:
		std::vector<unsigned int> VAOs;
		std::vector<unsigned int> VBOs;
		std::vector<int> edgeSampledPointsCounts;
		std::vector<glm::vec3> edgeColors;
		Shader shader;

		void Render(
			const RenderInfo& renderInfo
		) override {
			shader.use();

			shader.setMatrix4("projection", renderInfo.projection_matrix);
			shader.setMatrix4("view", renderInfo.view_matrix);
			shader.setMatrix4("model", renderInfo.model_matrix);
			//shader.setVec3("viewPos", camera_pos);

			for (int h = 0; h < VAOs.size(); h++) {
				int VAO = VAOs[h];
				int VAO_size = edgeSampledPointsCounts[h];
				glm::vec3 color = edgeColors[h];

				shader.setVec3("subcolor", color);
				glBindVertexArray(VAO);
				glDrawArrays(GL_LINE_STRIP, 0, VAO_size);
			}
		}

		void LoadFromSatInfo(SatInfo& satInfo) {
			VAOs.clear();
			VBOs.clear();
			edgeSampledPointsCounts = satInfo.edgeSampledPointsCounts;
			edgeColors = satInfo.edgeColors;
			
			for (auto vertices : satInfo.edgeSampledPoints) {
				unsigned VAO, VBO;
				glGenVertexArrays(1, &VAO);
				glGenBuffers(1, &VBO);

				glBindVertexArray(VAO);
				glBindBuffer(GL_ARRAY_BUFFER, VBO);
				glBufferData(GL_ARRAY_BUFFER, sizeof(float) * vertices.size(), vertices.data(), GL_STATIC_DRAW);

				glEnableVertexAttribArray(0);
				glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);

				glBindBuffer(GL_ARRAY_BUFFER, 0);
				glBindVertexArray(0);

				VAOs.emplace_back(VAO);
				VBOs.emplace_back(VBO);
			}
		}

		// ͨ���ƶ�ת��shader����Ȩ
		SatLineRenderer(Shader&& shader) :
			shader(std::move(shader))
		{}

		~SatLineRenderer() {}
	};


	void ProcessToggleKey(GLFWwindow* window, int key, bool& var, bool& varPressed) {
		if (glfwGetKey(window, key) == GLFW_PRESS && !varPressed) {
			var = !var;
			varPressed = true;

			std::cout << "showModel: " << var << std::endl;
		}
		if (glfwGetKey(window, key) == GLFW_RELEASE) {
			varPressed = false;
		}
	}

	class MouseController {

	public:
		void MouseCallback(GLFWwindow* window, double xposIn, double yposIn) {
			float xpos = static_cast<float>(xposIn);
			float ypos = static_cast<float>(yposIn);

			if (firstMouse) {
				lastX = xpos;
				lastY = ypos;
				firstMouse = false;
			}

			float xoffset = xpos - lastX;
			float yoffset = -ypos + lastY; // reversed since y-coordinates go from bottom to top
			lastX = xpos;
			lastY = ypos;

			camera.ProcessMouseMovement(xoffset, yoffset);
		}

		void ScrollCallback(GLFWwindow* window, double xoffset, double yoffset) {
			camera.ProcessMouseScroll(static_cast<float>(yoffset));
		}

		MouseController(Camera& icamera):
			lastX(0.0f),
			lastY(0.0f),
			firstMouse(true),
			camera(icamera)
		{}

	private:
		Camera& camera;
		bool firstMouse;
		float lastX;
		float lastY;
	};

	class KeyboardController {
	public:
		bool& showModel;
		bool showModelPressed;
		float keyboardMovementSpeed;

		void KeyboardProcessInput(GLFWwindow* window, float deltaTime) {
			if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
				glfwSetWindowShouldClose(window, true);

			if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS) {
				camera.ChangeMovementSpeed(0.1);
				std::cout << "MovementSpeed: " << camera.MovementSpeed << std::endl;
			}

			if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS) {
				camera.ChangeMovementSpeed(-0.1);
				std::cout << "MovementSpeed: " << camera.MovementSpeed << std::endl;
			}

			if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
				camera.ProcessKeyboard(FORWARD, deltaTime);
			if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
				camera.ProcessKeyboard(BACKWARD, deltaTime);
			if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
				camera.ProcessKeyboard(LEFT, deltaTime);
			if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
				camera.ProcessKeyboard(RIGHT, deltaTime);

			// Ϊ�˷���Զ�̵ĳ���
			if (glfwGetKey(window, GLFW_KEY_I) == GLFW_PRESS)
				camera.ProcessMouseMovement(0.0f, keyboardMovementSpeed);
			if (glfwGetKey(window, GLFW_KEY_K) == GLFW_PRESS)
				camera.ProcessMouseMovement(0.0f, -keyboardMovementSpeed);
			if (glfwGetKey(window, GLFW_KEY_J) == GLFW_PRESS)
				camera.ProcessMouseMovement(-keyboardMovementSpeed, 0.0f);
			if (glfwGetKey(window, GLFW_KEY_L) == GLFW_PRESS)
				camera.ProcessMouseMovement(keyboardMovementSpeed, 0.0f);

			ProcessToggleKey(window, GLFW_KEY_B, showModel, showModelPressed);
		}

		KeyboardController(Camera& icamera, bool& showModel) :
			camera(icamera),
			showModel(showModel),
			showModelPressed(false),
			keyboardMovementSpeed(5.0f)
		{}

	private:
		Camera& camera;
	};

	class MyRenderEngine {

	public:
		// settings
		unsigned int screenWidth;
		unsigned int screenHeight;
		glm::vec4 backgroundColor;
		bool showModel;

		GLFWwindow* window;
		Camera camera;
		MouseController mouseController;
		KeyboardController keyboardController;

		std::vector<std::shared_ptr<IRenderable>> IRenderables;

		// glfw: whenever the window size changed (by OS or user resize) this callback function executes
		void FramebufferSizeCallback(GLFWwindow* window, int width, int height) {
			// make sure the viewport matches the new window dimensions; note that width and 
			// height will be significantly larger than specified on retina displays.
			glViewport(0, 0, width, height);
		}

		void AddRenderable(const std::shared_ptr<IRenderable>& r) {
			IRenderables.emplace_back(r);
		}

		void SetCameraPos(const glm::vec3& pos) {
			camera.Position = pos;
		}

		void StartRenderLoop() {

			RenderInfo renderInfo;

			while (!glfwWindowShouldClose(window)) {
				// poll IO events (keys pressed/released, mouse moved etc.)
				glfwPollEvents();

				// Imgui settings first
				if (glfwGetWindowAttrib(window, GLFW_ICONIFIED) != 0)
				{
					ImGui_ImplGlfw_Sleep(10);
					continue;
				}

				ImGui_ImplOpenGL3_NewFrame();
				ImGui_ImplGlfw_NewFrame();
				ImGui::NewFrame();

				ImGui::ShowDemoWindow();

				// per-frame time logic
				float currentFrame = static_cast<float>(glfwGetTime());
				deltaTime = currentFrame - lastFrame;
				lastFrame = currentFrame;

				// input
				// -----
				keyboardController.KeyboardProcessInput(window, deltaTime);

				// render
				// -----
				int display_w, display_h;
				glfwGetFramebufferSize(window, &display_w, &display_h);
				glViewport(0, 0, display_w, display_h);

				glClearColor(backgroundColor.r, backgroundColor.g, backgroundColor.b, backgroundColor.a);
				glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

				glm::mat4 projectionMatrix = glm::perspective(glm::radians(camera.Zoom), static_cast<float>(screenWidth) / static_cast<float>(screenHeight), camera.Near, camera.Far);
				glm::mat4 viewMatrix = camera.GetViewMatrix();
				glm::mat4 modelMatrix{ 1.0f };

				renderInfo.projection_matrix = projectionMatrix;
				renderInfo.view_matrix = viewMatrix;
				renderInfo.model_matrix = modelMatrix;
				renderInfo.camera_pos = camera.Position;

				renderInfo.showModel = showModel;

				// render IRenderable
				for (auto&& r : IRenderables) {
					r->Render(renderInfo);
				}

				// Imgui rendering
				ImGui::Render();
				ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

				// glfw: swap buffers
				// -------------------------------------------------------------------------------
				glfwSwapBuffers(window);
			}

			// glfw: terminate, clearing all previously allocated GLFW resources.
			// ------------------------------------------------------------------
			glfwTerminate();
		}

		void SetupImGui() {
			// Setup Dear ImGui context
			IMGUI_CHECKVERSION();
			ImGui::CreateContext();
			ImGuiIO& io = ImGui::GetIO(); (void)io;
			io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
			io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

			// Setup Dear ImGui style
			ImGui::StyleColorsDark();
			//ImGui::StyleColorsLight();

			// Setup Platform/Renderer backends
			ImGui_ImplGlfw_InitForOpenGL(window, true);
			ImGui_ImplOpenGL3_Init("#version 130");
		}

		void SetupGlobalOpenglState() {
			glEnable(GL_DEPTH_TEST);
			//glEnable(GL_BLEND);
			//glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		}

		MyRenderEngine(): 
			camera(glm::vec3{100.0f, 100.0f, 100.0f}),
			mouseController(camera),
			keyboardController(camera, showModel),
			deltaTime(0.0f),
			lastFrame(0.0f),

			screenWidth(1024),
			screenHeight(768),
			backgroundColor(0.2f, 0.3f, 0.3f, 1.0f)
		{
			int init_res = InitWindow(window);
			if (init_res != 0) {
				throw std::runtime_error("Init window failed");
			}
			glfwMakeContextCurrent(window);
			glfwSwapInterval(1); // Enable vsync

			SetupImGui();
			SetupGlobalOpenglState();
		}

	private:
		float deltaTime;
		float lastFrame;

		int InitWindow(GLFWwindow* &window) {
			glfwInit();
			glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
			glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
			glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
			glfwWindowHint(GLFW_SAMPLES, 4);

#ifdef __APPLE__
			glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif
			window = glfwCreateWindow(screenWidth, screenHeight, "MySatViewer", NULL, NULL);
			if (window == nullptr) {
				std::cout << "Failed to create GLFW window" << std::endl;
				glfwTerminate();
				return -1;
			}
			glfwMakeContextCurrent(window);
			glfwSetWindowPos(window, 200, 200);

			auto framebufferSizeCallback = [](GLFWwindow* window, int width, int height) {
				MyRenderEngine* app = static_cast<MyRenderEngine*>(glfwGetWindowUserPointer(window));
				if (app) {
					app->FramebufferSizeCallback(window, width, height);
				}
			};

			auto mouseCallback = [](GLFWwindow* window, double xposIn, double yposIn) {
				MyRenderEngine* app = static_cast<MyRenderEngine*>(glfwGetWindowUserPointer(window));
				if (app) {
					app->mouseController.MouseCallback(window, xposIn, yposIn);
				}
			};

			auto scrollCallback = [](GLFWwindow* window, double xoffset, double yoffset) {
				MyRenderEngine* app = static_cast<MyRenderEngine*>(glfwGetWindowUserPointer(window));
				if (app) {
					app->mouseController.ScrollCallback(window, xoffset, yoffset);
				}
			};

			glfwSetWindowUserPointer(window, this);
			glfwSetFramebufferSizeCallback(window, framebufferSizeCallback);
			//glfwSetCursorPosCallback(window, mouseCallback); // ��ʱ�������
			glfwSetScrollCallback(window, scrollCallback);

			// glad: load all OpenGL function pointers
			// ---------------------------------------
			if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
			{
				std::cout << "Failed to initialize GLAD" << std::endl;
				return -1;
			}

			return 0;
		}
	};

	class MyGuiRenderer : public IRenderable {
	public:
		SatInfo& satInfo;

		void Render(
			const RenderInfo& renderInfo
		) override {

			ImGui::Begin("SAT Edges Info");
			if (ImGui::TreeNode("Edges")) {
				int i = 0;
				for (auto [edge_marknum, edge_info] : satInfo.edgeMap) {
					ImGui::PushID(i++);
					if (ImGui::TreeNode("", "Edge: %d", edge_marknum)) {

						ImGui::Text("Index: %d", edge_info.edgeIndex);
						ImGui::Text("Nonmanifold Count: %d", edge_info.nonmanifoldCount);
						ImGui::Text("Body ID: %d", edge_info.bodyId);
						ImGui::Text("Edge Mid Pos: (%d, %d, %d)", edge_info.edgeMidPos.x, edge_info.edgeMidPos.y, edge_info.edgeMidPos.z);
						if (ImGui::Button("Go")) {
							myRenderEngine.SetCameraPos(edge_info.edgeMidPos);
						}

						ImGui::TreePop();
					}
					ImGui::PopID();
				}

				ImGui::TreePop();
			}


			ImGui::End();
		}

		MyGuiRenderer(SatInfo& satInfo, MyRenderEngine& myRenderEngine) : satInfo(satInfo), myRenderEngine(myRenderEngine) {}

		~MyGuiRenderer() {}

	private:
		MyRenderEngine& myRenderEngine;
	};
} // namespace MyRenderEngine