#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include "shader_s.h"
#include "camera.h"

#include "model.h"
#include "mesh.h"

#include "stl_reader.h"
#include "json.hpp"
#include "argpaser.hpp"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <iostream>
#include <fstream>

using json = nlohmann::json;

const string VERSION = "0.1";

void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void mouse_callback(GLFWwindow* window, double xposIn, double yposIn);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void processInput(GLFWwindow* window);

unsigned int loadTexture(char const* path);
unsigned int loadCubemap(vector<std::string> faces);

int stlVAO;
int stlVerticesSize;

std::vector<int> lineVAOs;
std::vector<int> lineVerticesSize;
std::map<int, glm::vec3> lineMidPoses;
std::vector<glm::vec3> subcolors;

int Init(GLFWwindow*& w);
glm::vec3 GetEdgePos(int edge_id);

void setStlVAO(const std::string& stl_file_path);
void setLineVAO(const std::string& json_path, int selected_body = -1);

// settings
const unsigned int SCR_WIDTH = 1024;
const unsigned int SCR_HEIGHT = 768;

// 全局变量2：用于相机系统
glm::vec3 newCameraPos(100.0f, 100.0f, 100.0f);
Camera camera(newCameraPos);

float deltaTime = 0.0f;
float lastFrame = 0.0f;

bool firstMouse = true;
float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;

// lighting
glm::vec3 lightPos(0.0f, 0.0f, 0.0f);

// 全局变量：（用两个变量，确保在按下B松开后再次按下才会切换状态）
bool showModel = false;
bool showModelPressed = false;

int main(int argc, char const* argv[])
{
    // parse args
    auto args_parser = util::argparser("SAT Viewer by TML104");
    args_parser.set_program_name("MySatViewer")
        .add_help_option()
        .use_color_error()
        .add_sc_option("-v", "--version", "show version info", []() {std::cout << "MySatViewer version" << VERSION << std::endl; })
        .add_option<int>("-m", "--model", "Which body you want to show for lines.", -1)
        .add_option<int>("-t", "--tp", "TP to edge", -1)
        .add_argument<std::string>("stl_model_path", "stl model path")
        .add_argument<std::string>("geometry_json_path", "geometry json path")
        .parse(argc, argv);

    int selected_body = args_parser.get_option<int>("-m");
    int tp_to_edge = args_parser.get_option<int>("-t");
    std::string stl_model_path = args_parser.get_argument<std::string>("stl_model_path");
    std::string geometry_json_path = args_parser.get_argument<std::string>("geometry_json_path");

    // parse args END

    GLFWwindow* window;
    int init_res = Init(window);
    if (init_res != 0)
    {
        return -1;
    }

    // configure global opengl state
    // -----------------------------
    glEnable(GL_DEPTH_TEST);
    //glEnable(GL_BLEND);
    //glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    Shader stlShader("./shaders/MySat/flatShader.vs", "./shaders/MySat/flatShader.fs");

    Shader lineShader("./shaders/MySat/lineShader.vs", "./shaders/MySat/lineShader.fs");

    // load model
    setStlVAO(stl_model_path); // "./models/C_ent(1)_stl_2.stl"
    setLineVAO(geometry_json_path, selected_body); // "./models/C_ent(1)_geometry_json_.json"

    // change camera pos
    if (tp_to_edge == -1)
    {
        newCameraPos += glm::vec3(20.0f, 20.0f, 20.0f);
    }
    else
    {
        newCameraPos = GetEdgePos(tp_to_edge);
    }
    camera.Position = newCameraPos;

    // render loop
    // -----------
    while (!glfwWindowShouldClose(window))
    {
        // per-frame time logic
        float currentFrame = static_cast<float>(glfwGetTime());
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        // input
        // -----
        processInput(window);

        // render
        // ------
        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glm::mat4 cameraProjection = glm::perspective(glm::radians(camera.Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 1000.0f);
        glm::mat4 cameraView = camera.GetViewMatrix();
        glm::mat4 model{ 1.0f };
        // Stl model
        if (showModel) {
            stlShader.use();

            stlShader.setMatrix4("projection", cameraProjection);
            stlShader.setMatrix4("view", cameraView);
            stlShader.setMatrix4("model", model);
            stlShader.setVec3("viewPos", camera.Position);

            glBindVertexArray(stlVAO);
            glDrawArrays(GL_TRIANGLES, 0, stlVerticesSize);
        }

        // line
        lineShader.use();
        lineShader.setMatrix4("projection", cameraProjection);
        lineShader.setMatrix4("view", cameraView);
        lineShader.setMatrix4("model", model);
    
        for (int h = 0;h<lineVAOs.size();h++)
        {
            int lineVAO = lineVAOs[h];
            int VAOSize = lineVerticesSize[h];
            
            lineShader.setVec3("subcolor", subcolors[h]);
            glBindVertexArray(lineVAO);
            glDrawArrays(GL_LINE_STRIP, 0, VAOSize);
        }

        // glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
        // -------------------------------------------------------------------------------
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // glfw: terminate, clearing all previously allocated GLFW resources.
    // ------------------------------------------------------------------
    glfwTerminate();
    return 0;
}


int Init(GLFWwindow*& w)
{
    // glfw: initialize and configure
    // ------------------------------
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_SAMPLES, 4);

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    // glfw window creation
    // --------------------
    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "MySatViewer", NULL, NULL);
    w = window;
    if (window == NULL)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetWindowPos(window, 200, 200);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);

    // tell GLFW to capture our mouse
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    // glad: load all OpenGL function pointers
    // ---------------------------------------
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    return 0;
}

glm::vec3 GetEdgePos(int edge_id)
{
    return lineMidPoses[edge_id];
}

// 顺带更新一下位置
void setStlVAO(const std::string& stl_file_path)
{
    stl_reader::StlMesh <float, unsigned int> mesh(stl_file_path);

    std::vector<float> vertices;

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
                vertices.emplace_back(triangle_points[j].x);
                vertices.emplace_back(triangle_points[j].y);
                vertices.emplace_back(triangle_points[j].z);

                vertices.emplace_back(normalized_normal.x);
                vertices.emplace_back(normalized_normal.y);
                vertices.emplace_back(normalized_normal.z);

                newCameraPos.x = std::min(newCameraPos.x, triangle_points[j].x);
                newCameraPos.y = std::min(newCameraPos.y, triangle_points[j].y);
                newCameraPos.z = std::min(newCameraPos.z, triangle_points[j].z);
            }
        }
    }

    stlVerticesSize = vertices.size();

    unsigned int VAO, VBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(float) * vertices.size(), vertices.data(), GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    stlVAO = VAO;
}

void setLineVAO(const std::string& json_path, int selected_body)
{
    std::ifstream f(json_path);
    json data = json::parse(f);

    auto root_edges = data["root_edges"];

    for (auto&& element : root_edges) {
        int nonmanifold_count = element["nonmanifold_count"];
        int body_id = element["body"];
        int marknum = element["marknum"];

        if (selected_body != -1 && selected_body != body_id)
        {
            continue;
        }

        if (nonmanifold_count != 2)
        {
            printf("nonmanifold: edge: %d, body: %d, count: %d \n", int(element["marknum"]), body_id, nonmanifold_count);
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
        subcolors.emplace_back(color);

        auto&& sampled_points = element["sampled_points"];

        std::vector<float> vertices;
        unsigned int lineVAO, lineVBO;
        int point_count = 0;

        for (auto&& point: sampled_points) {
            float x = point["x"];
            float y = point["y"];
            float z = point["z"];

            //std::cout << x << " " << y << " " << z << " " << std::endl;
            vertices.emplace_back(x);
            vertices.emplace_back(y);
            vertices.emplace_back(z);

            point_count++;
        }

        // mid pos
        glm::vec3 mid_pos(0.0f, 0.0f, 0.0f);
        mid_pos.x = sampled_points[point_count / 2]["x"];
        mid_pos.y = sampled_points[point_count / 2]["y"];
        mid_pos.z = sampled_points[point_count / 2]["z"];

        lineMidPoses[marknum] = mid_pos;

        lineVerticesSize.emplace_back(point_count);

        glGenVertexArrays(1, &lineVAO);
        glGenBuffers(1, &lineVBO);

        glBindVertexArray(lineVAO);
        glBindBuffer(GL_ARRAY_BUFFER, lineVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(float) * vertices.size(), vertices.data(), GL_STATIC_DRAW);

        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);

        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);

        lineVAOs.emplace_back(lineVAO);
    }
}



// process all input: query GLFW whether relevant keys are pressed/released this frame and react accordingly
// ---------------------------------------------------------------------------------------------------------
void processInput(GLFWwindow* window)
{
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
    const float keyboardMovmentSpeed = 5.0f;

    if (glfwGetKey(window, GLFW_KEY_I) == GLFW_PRESS)
        camera.ProcessMouseMovement(0.0f, keyboardMovmentSpeed);
    if (glfwGetKey(window, GLFW_KEY_K) == GLFW_PRESS)
        camera.ProcessMouseMovement(0.0f, -keyboardMovmentSpeed);
    if (glfwGetKey(window, GLFW_KEY_J) == GLFW_PRESS)
        camera.ProcessMouseMovement(-keyboardMovmentSpeed, 0.0f);
    if (glfwGetKey(window, GLFW_KEY_L) == GLFW_PRESS)
        camera.ProcessMouseMovement(keyboardMovmentSpeed, 0.0f);

    if (glfwGetKey(window, GLFW_KEY_B) == GLFW_PRESS && !showModelPressed) {
        showModel = !showModel;
        showModelPressed = true;

        // [debug] blinn
        std::cout << "showModel: " << showModel << std::endl;
    }
    if (glfwGetKey(window, GLFW_KEY_B) == GLFW_RELEASE) {
        showModelPressed = false;
    }
}

// glfw: whenever the window size changed (by OS or user resize) this callback function executes
// ---------------------------------------------------------------------------------------------
void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    // make sure the viewport matches the new window dimensions; note that width and 
    // height will be significantly larger than specified on retina displays.
    glViewport(0, 0, width, height);
}

void mouse_callback(GLFWwindow* window, double xposIn, double yposIn) {

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

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset) {
    camera.ProcessMouseScroll(static_cast<float>(yoffset));
}

// utility function for loading a 2D texture from file
// 绑定纹理对象与实际数据，并返回纹理对象ID以供随后激活纹理单元并将纹理对象与纹理单元绑定
// ---------------------------------------------------
unsigned int loadTexture(char const* path) {
    unsigned int textureID;
    glGenTextures(1, &textureID);

    int width, height, nrComponents;
    unsigned char* data = stbi_load(path, &width, &height, &nrComponents, 0);

    if (data) {
        GLenum format;
        if (nrComponents == 1) {
            format = GL_RED;
        }
        else if (nrComponents == 3) {
            format = GL_RGB;
        }
        else if (nrComponents == 4) {
            format = GL_RGBA;
        }

        // 绑定：绑定纹理对象与实际数据
        glBindTexture(GL_TEXTURE_2D, textureID);
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);

        // 纹理环绕方式（与绑定之间的顺序随意）
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, format == GL_RGBA ? GL_CLAMP_TO_EDGE : GL_REPEAT); // for this tutorial: use GL_CLAMP_TO_EDGE to prevent semi-transparent borders. Due to interpolation it takes texels from next repeat 
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, format == GL_RGBA ? GL_CLAMP_TO_EDGE : GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        stbi_image_free(data);

    }
    else {
        std::cout << "Texture failed to load at path:" << path << std::endl;
        stbi_image_free(data);
    }

    return textureID;
}

// loads a cubemap texture from 6 individual texture faces
// order:
// +X (right)
// -X (left)
// +Y (top)
// -Y (bottom)
// +Z (front) 
// -Z (back)
// -------------------------------------------------------
unsigned int loadCubemap(vector<std::string> faces)
{
    unsigned int textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_CUBE_MAP, textureID);

    int width, height, nrChannels;
    for (unsigned int i = 0; i < faces.size(); i++)
    {
        unsigned char* data = stbi_load(faces[i].c_str(), &width, &height, &nrChannels, 0);
        if (data)
        {
            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
            stbi_image_free(data);
        }
        else
        {
            std::cout << "Cubemap texture failed to load at path: " << faces[i] << std::endl;
            stbi_image_free(data);
        }
    }
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    return textureID;
}