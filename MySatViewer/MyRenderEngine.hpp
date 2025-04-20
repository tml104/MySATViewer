#pragma once

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <iostream>
#include <fstream>
#include <memory>
#include <unordered_map>
#include <unordered_set>
#include <set>
#include <vector>
#include <cmath>

#include "Utils.hpp"
#include "Configs.hpp"

#include "shader_s.h"
#include "camera.h"

#include "stl_reader.h"
#include "json.hpp"
#include "tiny_obj_loader.h"

#include "IRenderable.hpp"

#include "Event.hpp"
#include "Dispatcher.hpp"
#include "SetCameraPosEvent.hpp"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

using json = nlohmann::json;

namespace MyRenderEngine {

	// 这里就不用RenderableInfo这个设计了，因为之前里面只是用来传递isOpaque的，而这个量只在调用myRenderEngine.AddRenderable的时候有用。现在需要改成能够动态调整的，所以就不用了

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

			// 为了方便远程的尝试
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
		unsigned int screenXPos;
		unsigned int screenYPos;

		glm::vec4 backgroundColor;
		bool showModel;
		float scaleFactor;
		bool transparentModel;

		GLFWwindow* window;
		Camera camera;
		MouseController mouseController;
		KeyboardController keyboardController;

		// 渲染对象列表
		// opaqueRenderables：固定渲染在不透明framebuffer上的， opaqueOrTransparentRenderables： 可能渲染在不透明或透明framebuffer上的（通过transparentModel切换）
		std::vector<std::shared_ptr<IRenderable>> opaqueRenderables, opaqueOrTransparentRenderables, guiRenderables;

		std::shared_ptr<IRenderable> screenQuad; // [后设置]

		// Framebuffer
		// 生命周期已经闭环
		unsigned int opaqueFBO;
		unsigned int transparentFBO;

		unsigned int opaqueTexture;
		unsigned int depthTexture;

		unsigned int accumTexture;
		unsigned int revealTexture;

		// OIT Use Shaders
		Shader* compositeShader;
		Shader* screenShader;

		// glfw: whenever the window size changed (by OS or user resize) this callback function executes
		void FramebufferSizeCallback(GLFWwindow* window, int width, int height) {
			// make sure the viewport matches the new window dimensions; note that width and 
			// height will be significantly larger than specified on retina displays.
			glViewport(0, 0, width, height);
		}
		
		void AddOpaqueRenderable(const std::shared_ptr<IRenderable>& r) {
			opaqueRenderables.emplace_back(r);
		}

		void AddOpaqueOrTransparentRenderable(const std::shared_ptr<IRenderable>& r) {
			opaqueOrTransparentRenderables.emplace_back(r);
		}

		void AddScreenQuadRenderable(const std::shared_ptr<IRenderable>& r) {
			screenQuad = r;
		}

		void AddGuiRenderable(const std::shared_ptr<IRenderable>& r) {
			guiRenderables.emplace_back(r);
		}

		void SetCameraPos(const glm::vec3& pos) {
			camera.Position = pos;
		}

		void SetCompositeShader(Shader* shader) {
			compositeShader = shader;
		}

		void SetScreenShader(Shader* shader) {
			screenShader = shader;
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

				for (auto& r : guiRenderables) {
					r->Render(renderInfo);
				}


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

				glm::mat4 projectionMatrix = glm::perspective(glm::radians(camera.Zoom), static_cast<float>(screenWidth) / static_cast<float>(screenHeight), camera.Near, camera.Far);
				glm::mat4 viewMatrix = camera.GetViewMatrix();

				renderInfo.projectionMatrix = projectionMatrix;
				renderInfo.viewMatrix = viewMatrix;
				renderInfo.cameraPos = camera.Position;

				renderInfo.showModel = showModel;
				renderInfo.transparentModel = transparentModel;
				renderInfo.scaleFactor = scaleFactor;

				// render IRenderable to opaqueFBO & transparentFBO
				// -> Opaque (solid pass)
				glEnable(GL_DEPTH_TEST);
				glDepthFunc(GL_LESS);
				glDepthMask(GL_TRUE);
				glDisable(GL_BLEND);
				glClearColor(backgroundColor.r, backgroundColor.g, backgroundColor.b, backgroundColor.a);

				glBindFramebuffer(GL_FRAMEBUFFER, opaqueFBO);
				glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

				for (auto& r : opaqueRenderables) {
					r->Render(renderInfo);
				}

				if (transparentModel == false) {
					for (auto& r : opaqueOrTransparentRenderables) {
						r->Render(renderInfo);
					}
				}

				// -> Transparent (transparent pass)
				glDepthMask(GL_FALSE);
				glEnable(GL_BLEND);
				glBlendFunci(0, GL_ONE, GL_ONE);
				glBlendFunci(1, GL_ZERO, GL_ONE_MINUS_SRC_COLOR);
				glBlendEquation(GL_FUNC_ADD);

				glBindFramebuffer(GL_FRAMEBUFFER, transparentFBO);
				glClearBufferfv(GL_COLOR, 0, &Configs::ZERO_VEC[0]); // 新函数
				glClearBufferfv(GL_COLOR, 1, &Configs::ONE_VEC[0]);

				if (transparentModel == true) {
					for (auto& r : opaqueOrTransparentRenderables) {
						r->Render(renderInfo);
					}
				}

				// render composite image (composite pass)
				glDepthFunc(GL_ALWAYS);
				glEnable(GL_BLEND);
				glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

				glBindFramebuffer(GL_FRAMEBUFFER, opaqueFBO);

				compositeShader->use();
				glActiveTexture(GL_TEXTURE0);
				glBindTexture(GL_TEXTURE_2D, accumTexture);
				glActiveTexture(GL_TEXTURE1);
				glBindTexture(GL_TEXTURE_2D, revealTexture);

				screenQuad->Render(renderInfo);

				// render screenQuad (draw to backbuffer) (final pass)
				glDisable(GL_DEPTH_TEST);
				glDepthMask(GL_TRUE); // enable depth writes so glClear won't ignore clearing the depth buffer
				glDisable(GL_BLEND);

				glBindFramebuffer(GL_FRAMEBUFFER, 0);
				glClearColor(backgroundColor.r, backgroundColor.g, backgroundColor.b, backgroundColor.a);
				glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

				screenShader->use();
				glActiveTexture(GL_TEXTURE0);
				glBindTexture(GL_TEXTURE_2D, opaqueTexture);

				screenQuad->Render(renderInfo);

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

		// 这个没用上
		void SetupGlobalOpenglState() {
			glEnable(GL_DEPTH_TEST);
			//glEnable(GL_BLEND);
			//glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

			glDepthFunc(GL_LESS); // 深度缓冲比较通过条件：小于 （默认就是这个吧）
			glDepthMask(GL_TRUE); // 允许更新深度缓冲
			glDisable(GL_BLEND);
			glClearColor(backgroundColor.r, backgroundColor.g, backgroundColor.b, backgroundColor.a);
		}

		void SetupFrameBuffers() {
			glGenFramebuffers(1, &opaqueFBO);
			glGenFramebuffers(1, &transparentFBO);

			// opaqueTexture
			glGenTextures(1, &opaqueTexture);
			glBindTexture(GL_TEXTURE_2D, opaqueTexture);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, Configs::SCR_WIDTH, Configs::SCR_HEIGHT, 0, GL_RGBA, GL_HALF_FLOAT, NULL); // 注意这里用的是 gl_half_float
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			glBindTexture(GL_TEXTURE_2D, 0);

			// depthTexture
			glGenTextures(1, &depthTexture);
			glBindTexture(GL_TEXTURE_2D, depthTexture);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, Configs::SCR_WIDTH, Configs::SCR_HEIGHT,
				0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
			glBindTexture(GL_TEXTURE_2D, 0);

			// Bind opaqueTexture & depthTexture to opaqueFBO
			glBindFramebuffer(GL_FRAMEBUFFER, opaqueFBO);
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, opaqueTexture, 0);
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthTexture, 0);

			if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
				std::cout << "ERROR::FRAMEBUFFER:: Opaque framebuffer is not complete!" << std::endl;

			glBindFramebuffer(GL_FRAMEBUFFER, 0);

			// set up attachments for transparent framebuffer
			// accumTexture
			glGenTextures(1, &accumTexture);
			glBindTexture(GL_TEXTURE_2D, accumTexture);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, Configs::SCR_WIDTH, Configs::SCR_HEIGHT, 0, GL_RGBA, GL_HALF_FLOAT, NULL);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			glBindTexture(GL_TEXTURE_2D, 0);

			// revealTexture
			glGenTextures(1, &revealTexture);
			glBindTexture(GL_TEXTURE_2D, revealTexture);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, Configs::SCR_WIDTH, Configs::SCR_HEIGHT, 0, GL_RED, GL_FLOAT, NULL); // 注意这里因为只有一个通道所以会有所不同！
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			glBindTexture(GL_TEXTURE_2D, 0);

			// Bind accumTexture & revealTexture & depthTexture to transparent
			glBindFramebuffer(GL_FRAMEBUFFER, transparentFBO);
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, accumTexture, 0);
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, revealTexture, 0);
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthTexture, 0); // opaque framebuffer's depth texture

			// don't forget to explicitly tell OpenGL that your transparent framebuffer has two draw buffers
			const GLenum transparentDrawBuffers[] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1 };
			glDrawBuffers(2, transparentDrawBuffers);

			if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
				std::cout << "ERROR::FRAMEBUFFER:: Transparent framebuffer is not complete!" << std::endl;

			glBindFramebuffer(GL_FRAMEBUFFER, 0);
		}


		void SetupEvents() {
			auto& dispatcher = EventSystem::Dispatcher::GetInstance();

			dispatcher.Subscribe(EventSystem::EventType::SetCameraPos, [this](const EventSystem::Event& e) {
				auto setCameraPosEvent = static_cast<const EventSystem::SetCameraPosEvent&>(e);
				SetCameraPos(setCameraPosEvent.pos);
			});
		}

		MyRenderEngine(): 
			camera(Configs::CAMERA_INIT_POS),
			mouseController(camera),
			keyboardController(camera, showModel),
			deltaTime(0.0f),
			lastFrame(0.0f),

			screenWidth(Configs::SCR_WIDTH),
			screenHeight(Configs::SCR_HEIGHT),
			screenXPos(Configs::SCR_X_POS),
			screenYPos(Configs::SCR_Y_POS),
			backgroundColor(Configs::BLACK_BACKGROUND),
			scaleFactor(1.0f),
			transparentModel(false),
			showModel(false)
		{
			int init_res = InitWindow(window);
			if (init_res != 0) {
				throw std::runtime_error("Init window failed");
			}
			glfwMakeContextCurrent(window);
			glfwSwapInterval(1); // Enable vsync

			SetupImGui();
			//SetupGlobalOpenglState();
			SetupFrameBuffers();
			SetupEvents();
		}

		~MyRenderEngine() {
			glDeleteTextures(1, &opaqueTexture);
			glDeleteTextures(1, &depthTexture);
			glDeleteTextures(1, &accumTexture);
			glDeleteTextures(1, &revealTexture);
			glDeleteFramebuffers(1, &opaqueFBO);
			glDeleteFramebuffers(1, &transparentFBO);

			std::cout << "MyrenderEngine Destructor executed." << std::endl;
		}

	private:
		float deltaTime;
		float lastFrame;

		int InitWindow(GLFWwindow* &window) {
			glfwInit();
			glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
			glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
			glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
			//glfwWindowHint(GLFW_SAMPLES, 4); // 注意因为这里用了别的framebuffer所以这里没有意义
			glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GL_TRUE);

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
			//glfwSetCursorPosCallback(window, mouseCallback); // 暂时禁用鼠标
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

} // namespace MyRenderEngine