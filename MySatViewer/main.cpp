// MySatViewer.exe -m 2 ./models/C_ent(1)_stl_1.stl ./models/C_ent(1)_geometry_json_.json

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include "shader_s.h"
#include "camera.h"

#include "model.h"
#include "mesh.h"

#include "MyRenderEngine.hpp"

#include "stl_reader.h"
#include "json.hpp"
#include "argparser.hpp"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <iostream>
#include <fstream>

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

using json = nlohmann::json;

const string VERSION = "0.1";

unsigned int loadTexture(char const* path);
unsigned int loadCubemap(vector<std::string> faces);

int main(int argc, char const* argv[])
{
    // parse args
    auto args_parser = util::argparser("SAT Viewer by TML104");
    args_parser.set_program_name("MySatViewer")
        .add_help_option()
        .use_color_error()
        .add_sc_option("-v", "--version", "show version info", []() {std::cout << "MySatViewer version" << VERSION << std::endl; })
        .add_option<int>("-m", "--model", "Which body you want to show for lines.", -1)
        .add_option<std::string>("-o", "--obj", "Load Obj", "")
        .add_argument<std::string>("stl_model_path", "stl model path")
        .add_argument<std::string>("geometry_json_path", "geometry json path")
        .parse(argc, argv);

    int selected_body = args_parser.get_option<int>("-m");
    std::string obj_path = args_parser.get_option<std::string>("-o");
    std::string stl_model_path = args_parser.get_argument<std::string>("stl_model_path");
    std::string geometry_json_path = args_parser.get_argument<std::string>("geometry_json_path");

    // parse args END

    // 注意：myRenderEngine 必须先构造
    MyRenderEngine::MyRenderEngine myRenderEngine;
    
    Shader stlShader("./shaders/MySat/flatShader.vs", "./shaders/MySat/flatShader.fs");
    Shader objShader("./shaders/MySat/objShader.vs", "./shaders/MySat/objShader.fs");
    Shader objLineShader("./shaders/MySat/objLineShader.vs", "./shaders/MySat/objLineShader.fs");
    Shader lineShader("./shaders/MySat/lineShader.vs", "./shaders/MySat/lineShader.fs");

    MyRenderEngine::SatInfo satInfo;

    MyRenderEngine::ObjInfo objInfo;

    if (obj_path != "") {
        std::cout << "Loading OBJ: " << obj_path << std::endl;
        objInfo.LoadObj(obj_path); // 注意这个对象的生命周期，因为目前objRendererPtr是通过引用的方式拿信息的！
        std::cout << "Loading OBJ Done." << std::endl;

        //auto objRendererPtr = std::make_shared<MyRenderEngine::ObjRenderer>(objInfo ,std::move(objShader));
        //objRendererPtr->Setup();
        //myRenderEngine.AddRenderable(objRendererPtr);

        myRenderEngine.camera.MovementSpeed = 1.0f;

        auto objLineRendererPtr = std::make_shared<MyRenderEngine::ObjLineRenderer>(objInfo, std::move(objLineShader));
        objLineRendererPtr->Setup();
        myRenderEngine.AddRenderable(objLineRendererPtr);
    }
    else {
        std::cout << "Loading STL: " << stl_model_path << std::endl;
        satInfo.LoadStl(stl_model_path);
        std::cout << "Loading STL Done." << std::endl;

        std::cout << "Loading GeometryJson: " << geometry_json_path << std::endl;
        satInfo.LoadGeometryJson(geometry_json_path, selected_body);
        std::cout << "Loading GeometryJson Done." << std::endl;

        myRenderEngine.SetCameraPos(satInfo.newCameraPos);

        auto satStlRendererPtr = std::make_shared<MyRenderEngine::SatStlRenderer>(std::move(stlShader));
        satStlRendererPtr->LoadFromSatInfo(satInfo);
        myRenderEngine.AddRenderable(satStlRendererPtr);

        auto satLineRendererPtr = std::make_shared<MyRenderEngine::SatLineRenderer>(std::move(lineShader));
        satLineRendererPtr->LoadFromSatInfo(satInfo);
        myRenderEngine.AddRenderable(satLineRendererPtr);

        auto myGuiRendererPtr = std::make_shared<MyRenderEngine::MyGuiRenderer>(satInfo, myRenderEngine);
        myRenderEngine.AddRenderable(myGuiRendererPtr);
    }

    myRenderEngine.StartRenderLoop();

    return 0;
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