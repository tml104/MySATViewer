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

const string VERSION = "2024��10��30�� 15:15:58";

unsigned int loadTexture(char const* path);
unsigned int loadCubemap(vector<std::string> faces);

// -m 3 -o ./models/bunny.obj ./models/C_ent(1)_stl_2.stl ./models/C_ent(1)_geometry_json_.json

int main(int argc, char const* argv[])
{
    // parse args
    auto args_parser = util::argparser("SAT Viewer by TML104");
    args_parser.set_program_name("MySatViewer")
        .add_help_option()
        .use_color_error()
        .add_sc_option("-v", "--version", "show version info", []() {std::cout << "MySatViewer version: " << VERSION << std::endl; })
        .add_option("-o", "--obj", "Read from OBJ")
        .add_option("-s", "--stl", "Read from STL")
        .add_option<int>("-b", "--body", "(Only For STL) Which body you want to show for lines.", -1)
        .add_option<float>("-x", "--scale", "(Only For OBJ) Scale OBJ", 1.0)
        .add_option<double>("-D", "--distance", "(Only For OBJ) Distance Threshold", 0.001)
        .add_option<double>("-A", "--angle", "(Only For OBJ) Angle Threshold", 150.0)
        .add_option<std::string>("-p", "--path", "OBJ or STL Path", "")
        .add_option<std::string>("-g", "--geometry", "(Only For STL) Geometry File Path", "")
        .parse(argc, argv);

    // ģʽ
    bool obj_mode = args_parser.has_option("--obj");
    bool stl_mode = args_parser.has_option("--stl");

    int selected_body = args_parser.get_option<int>("-b");
    float scale_factor = args_parser.get_option<float>("-x");
    double distance_threshold = args_parser.get_option<double>("-D");
    double angle_threshold = args_parser.get_option<double>("-A");
    //std::string obj_path = args_parser.get_option<std::string>("-o");
    //std::string stl_model_path = args_parser.get_argument<std::string>("stl_model_path");
    //std::string geometry_json_path = args_parser.get_argument<std::string>("geometry_json_path");
    std::string model_path = args_parser.get_option<std::string>("-p");
    std::string geometry_path = args_parser.get_option<std::string>("-g");

    // parse args END

    // ע�⣺myRenderEngine �����ȹ���
    MyRenderEngine::MyRenderEngine myRenderEngine;
    
    Shader stlShader("./shaders/MySat/flatShader.vs", "./shaders/MySat/flatShader.fs");
    Shader objShader("./shaders/MySat/objShader.vs", "./shaders/MySat/objShader.fs");
    Shader objLineShader("./shaders/MySat/objLineShader2.vs", "./shaders/MySat/objLineShader2.fs");
    Shader lineShader("./shaders/MySat/lineShader.vs", "./shaders/MySat/lineShader.fs");

    MyRenderEngine::SatInfo satInfo;
    MyRenderEngine::ObjInfo objInfo;// ע���������������������. ���ﲻ�ܰ�����������Ų��if���棬��ΪĿǰobjRendererPtr��ͨ�����õķ�ʽ����Ϣ�ģ�

    if (obj_mode) {
        std::cout << "Loading OBJ: " << model_path << std::endl;
        objInfo.LoadObj(model_path); 
        std::cout << "Loading OBJ Done." << std::endl;

        // Temp: ObjRenderer
        //auto objRendererPtr = std::make_shared<MyRenderEngine::ObjRenderer>(objInfo ,std::move(objShader));
        //objRendererPtr->Setup();
        //myRenderEngine.AddRenderable(objRendererPtr);

        //myRenderEngine.camera.MovementSpeed = 1.0f;
        MyRenderEngine::ObjLineWithGuiRendererInputs inputs{ scale_factor , distance_threshold, angle_threshold };

        auto objLineWithGuiRendererPtr = std::make_shared<MyRenderEngine::ObjLineWithGuiRenderer>(objInfo, inputs, std::move(objLineShader), myRenderEngine);
        objLineWithGuiRendererPtr->Setup();
        myRenderEngine.AddRenderable(objLineWithGuiRendererPtr);
    }
    else if(stl_mode) {
        std::cout << "Loading STL: " << model_path << std::endl;
        satInfo.LoadStl(model_path);
        std::cout << "Loading STL Done." << std::endl;

        std::cout << "Loading Geometry Json: " << geometry_path << std::endl;
        satInfo.LoadGeometryJson(geometry_path, selected_body);
        std::cout << "Loading Geometry Json Done." << std::endl;

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
// �����������ʵ�����ݣ��������������ID�Թ���󼤻�����Ԫ�����������������Ԫ��
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

        // �󶨣������������ʵ������
        glBindTexture(GL_TEXTURE_2D, textureID);
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);

        // �����Ʒ�ʽ�����֮���˳�����⣩
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