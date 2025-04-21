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

#include <spdlog/spdlog.h>
#include <spdlog/sinks/basic_file_sink.h>

#include "BasicGuiRenderer.hpp"
#include "ScreenQuad.hpp"

#include "SatInfo.hpp"
#include "ObjInfo.hpp"
#include "DebugShowInfo.hpp"

#include "ObjMarkNum.hpp"

#include "ObjRenderer.hpp"
#include "ObjLineRenderer.hpp"
#include "ObjGuiRenderer.hpp"

#include "SatStlRenderer.hpp"
#include "SatLineRenderer.hpp"
#include "SatGuiRenderer.hpp"

#include "DebugShowRenderer.hpp"
#include "DebugShowGuiRenderer.hpp"

using json = nlohmann::json;

#define COMPILE_DATETIME (__DATE__ " " __TIME__)
const string VERSION = COMPILE_DATETIME;

// -m 3 -o ./models/bunny.obj ./models/C_ent(1)_stl_2.stl ./models/C_ent(1)_geometry_json_.json

// -o -p ./models/bunny.obj

// -s -p ./models/rot_cyl3_stl_0.stl -g ./models/rot_cyl3_geometry_json_.json

// -o -p ./models/bunny.obj -d ./models/A_ent1(1)_cf_debugshow.json

// -s -p ./models/A_ent1(1)_cf_stl_0.stl -g ./models/A_ent1(1)_cf_geometry_json_0.json -d ./models/A_ent1(1)_cf_debugshow.json

void SetSpdlogPattern(std::string file_name = "default")
{
    //spdlog::set_pattern("[%H:%M:%S %z] [%n] [%^%L%$] [thread %t] [%s] [%@] %v");

    // set formatter
	std::string log_name = "logs/log_" + file_name + ".log";

    auto file_logger = spdlog::basic_logger_mt("default_logger", log_name, true);
    spdlog::set_default_logger(file_logger);

    spdlog::set_pattern("[%H:%M:%S %z] [%^%L%$] [thread %t] [%s] [%@] [%!] %v");
    spdlog::set_level(spdlog::level::trace);
}

int main(int argc, char const* argv[])
{
	SetSpdlogPattern();
    SPDLOG_INFO("test");

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
		.add_option<std::string>("-d", "--debugshow", "DebugShow File Path", "")
        .parse(argc, argv);

    // 模式
    bool obj_mode = args_parser.has_option("--obj");
    bool stl_mode = args_parser.has_option("--stl");

    int selected_body = args_parser.get_option<int>("-b");
    float scale_factor = args_parser.get_option<float>("-x");
    double distance_threshold = args_parser.get_option<double>("-D");
    double angle_threshold = args_parser.get_option<double>("-A");
    std::string model_path = args_parser.get_option<std::string>("-p");
    std::string geometry_path = args_parser.get_option<std::string>("-g");
	std::string debugshow_path = args_parser.get_option<std::string>("-d");

    // parse args END

    // 注意：myRenderEngine 必须先构造
    MyRenderEngine::MyRenderEngine myRenderEngine;
    
    Shader stlShader("./shaders/MySat/OIT/stlShader.vs", "./shaders/MySat/OIT/stlShader.fs");
    Shader objShader("./shaders/MySat/OIT/objShader.vs", "./shaders/MySat/OIT/objShader.fs");

    Shader stlTransparentShader("./shaders/MySat/OIT/stlShader.vs", "./shaders/MySat/OIT/stlTransparentShader.fs");
    Shader objTransparentShader("./shaders/MySat/OIT/objShader.vs", "./shaders/MySat/OIT/objTransparentShader.fs");

    Shader objLineShader("./shaders/MySat/OIT/objLineShader.vs", "./shaders/MySat/OIT/objLineShader.fs");
    Shader lineShader("./shaders/MySat/OIT/lineShader.vs", "./shaders/MySat/OIT/lineShader.fs");

    Shader compositeShader("./shaders/MySat/OIT/composite.vs", "./shaders/MySat/OIT/composite.fs");
    Shader screenShader("./shaders/MySat/OIT/screen.vs", "./shaders/MySat/OIT/screen.fs");

	Shader debugShowPointShader("./shaders/MySat/OIT/debugShowPointShader.vs", "./shaders/MySat/OIT/debugShowPointShader.fs");

    auto basicGuiRendererPtr = std::make_shared<MyRenderEngine::BasicGuiRenderer>(myRenderEngine);
    myRenderEngine.AddGuiRenderable(basicGuiRendererPtr);

    auto screenQuadRendererPtr = std::make_shared<MyRenderEngine::ScreenQuad>();
    myRenderEngine.AddScreenQuadRenderable(screenQuadRendererPtr);

    myRenderEngine.SetCompositeShader(&compositeShader);
    myRenderEngine.SetScreenShader(&screenShader);

    Info::SatInfo satInfo;
    Info::ObjInfo objInfo;// 注意这两个对象的生命周期. 这里不能把这两个对象挪到if里面，因为目前objRendererPtr是通过引用的方式拿信息的！
    Info::DebugShowInfo debugShowInfo;

    if (obj_mode) {
        std::cout << "Loading OBJ: " << model_path << std::endl;
        objInfo.LoadFromObj(model_path); 
        std::cout << "Loading OBJ Done." << std::endl;

        ObjMarkNum::GetInstance().LoadFromObjInfo(objInfo); // 注意这个必须先load

        auto objRendererPtr = std::make_shared<MyRenderEngine::ObjRenderer>(objInfo ,&(objShader), &(objTransparentShader));
        objRendererPtr->Setup();
        myRenderEngine.AddOpaqueOrTransparentRenderable(objRendererPtr);

        //auto objNonManifoldLineWithGuiRendererPtr = std::make_shared<MyRenderEngine::ObjNonManifoldLineWithGuiRenderer>(MyRenderEngine::ObjMarkNum::GetInstance(), &(objLineShader), myRenderEngine);
        //objNonManifoldLineWithGuiRendererPtr->SetUp();
        //myRenderEngine.AddOpaqueRenderable(objNonManifoldLineWithGuiRendererPtr);

		auto objLineRendererPtr = std::make_shared<MyRenderEngine::ObjLineRenderer>(&(objLineShader));
        objLineRendererPtr->SetUp();
		myRenderEngine.AddOpaqueRenderable(objLineRendererPtr);

		auto objGuiRendererPtr = std::make_shared<MyRenderEngine::ObjGuiRenderer>();
        objGuiRendererPtr->SetUp();
		myRenderEngine.AddGuiRenderable(objGuiRendererPtr);
    }
    else if(stl_mode) {
        std::cout << "Loading STL: " << model_path << std::endl;
        satInfo.LoadStl(model_path);
        std::cout << "Loading STL Done." << std::endl;

        std::cout << "Loading Geometry Json: " << geometry_path << std::endl;
        satInfo.LoadGeometryJson(geometry_path, selected_body);
        std::cout << "Loading Geometry Json Done." << std::endl;

        myRenderEngine.SetCameraPos(satInfo.newCameraPos);

        auto satStlRendererPtr = std::make_shared<MyRenderEngine::SatStlRenderer>(&(stlShader), &(stlTransparentShader));
        satStlRendererPtr->LoadFromSatInfo(satInfo);
        myRenderEngine.AddOpaqueOrTransparentRenderable(satStlRendererPtr);

        auto satLineRendererPtr = std::make_shared<MyRenderEngine::SatLineRenderer>(&(lineShader));
        satLineRendererPtr->LoadFromSatInfo(satInfo);
        myRenderEngine.AddOpaqueRenderable(satLineRendererPtr);

        auto myGuiRendererPtr = std::make_shared<MyRenderEngine::SatGuiRenderer>(satInfo);
        myRenderEngine.AddGuiRenderable(myGuiRendererPtr);
    }


    if (debugshow_path != "") {
        debugShowInfo.LoadFromDebugShowJson(debugshow_path);

		auto debugShowRendererPtr = std::make_shared<MyRenderEngine::DebugShowRenderer>(&debugShowPointShader);
		debugShowRendererPtr->LoadFromDebugShowInfo(debugShowInfo);

		myRenderEngine.AddOpaqueRenderable(debugShowRendererPtr);


		auto debugShowGuiRendererPtr = std::make_shared<MyRenderEngine::DebugShowGuiRenderer>(debugShowInfo);
		myRenderEngine.AddGuiRenderable(debugShowGuiRendererPtr);
    }

    myRenderEngine.StartRenderLoop();

    return 0;
}