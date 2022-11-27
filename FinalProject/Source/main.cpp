#include "imgui.h"
#include "imgui_impl_sdl.h"
#include "imgui_impl_sdlrenderer.h"
#include <stdio.h>
#include <iostream>
#include <SDL.h>

#include <fstream>
#include <filesystem>
#include <string>
#include <thread>
#include <vector>

#include <Application/Application.h>
#include <Maths/Ray.h>
#include <Maths/Vector3f.h>
#include <Renderer/Texture.h>
#include <Renderer/Camera.h>
#include <Renderer/Sphere.h>
#include <Renderer/Mesh.h>

#include <Error.h>

#include <nfd.h>

#include <algorithm>

#include <Renderer/Line.h>

using namespace Prototype;

struct RayVisualizationSettings {
    int maxDepth = 4;
    int initialRays = 8;
    float maxDistance = 1.5;
    Colour lineColour = { 1, 0, 0 };
};

class PrototypeApp : public Application {
public:
    PrototypeApp(const char* file) : Application("Ray Tracing Optics Simulator", 1900 * 0.75, 1080 * 0.75) {
        darkMode = true;
        SetUpImGui();

        width = 450 * 1;
        height = 225 * 1;
        aspectRatio = (float)width / (float)height;

        // Need to use an array as texture data wraps around after 255!
        image = new Colour * [width];
        isSelectedObjectVisible = new bool * [width];
        depthMap = new float * [width];
        for (int i = 0; i < width; i++) {
            image[i] = new Colour[height];
            isSelectedObjectVisible[i] = new bool[height];
            depthMap[i] = new float[height];
        }

        renderSettings = RenderSettings();
        renderSettings.resolution = { (float)width, (float)height };
        renderSettings.maxDepth = 10;
        renderSettings.samples = 1;
        renderSettings.ambientLight = Vector3f(1, 1, 1);
        renderSettings.checkerboard = false;
        renderSettings.directLightSampling = true;

        ambientLightColour = { (float)renderSettings.ambientLight.x, (float)renderSettings.ambientLight.y, (float)renderSettings.ambientLight.z, 1 };

        redraw = false;
        loadedNewFile = true;

        // The image which will be rendered on the GUI.
        finalImage = new Texture(width, height);
        finalImage2 = SDL_CreateTexture(Application::GetApp()->GetRenderer()->GetRawRenderer(), SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, width, height);

        SDL_SetTextureScaleMode(finalImage2, SDL_ScaleModeBest);

        if (file) std::cout << file << std::endl;
        if (!file) GenerateScene();
        else scene = scene.LoadFromFile(file);

        selectedObject = -1;
        newObjectType = 0;
        newObjectPath = "";
        newObjectName = "Object " + std::to_string(scene.GetObjectCount());

        showVisualizualisation = false;

        GenerateLines();
    }

    void GenerateScene() {
        // Generate a test scene.
        srand(2);

        Material ground = Material(MaterialType::Lambertian, { 0.5, 0.5, 0.5 });
        scene.AddObject("Ground", (Object*) new Sphere({ 0, -1000, -2 }, 1000, ground));


        // Add random objects.
        for (int i = -1; i <= 1; i += 1) {
            for (int j = -1; j <= 1; j += 1) {
                Material mat;
                switch (rand() % 4) {
                case 0:
                    mat = Material(MaterialType::Lambertian, { rand() / (RAND_MAX + 1.0f) * 0.9f + 0.1f , rand() / (RAND_MAX + 1.0f) * 0.9f + 0.1f, rand() / (RAND_MAX + 1.0f) * 0.9f + 0.1f });
                    break;

                case 1:
                    mat = Material(MaterialType::Glossy, { rand() / (RAND_MAX + 1.0f * 0.9f + 0.1f) , rand() / (RAND_MAX + 1.0f * 0.9f + 0.1f) , rand() / (RAND_MAX + 1.0f) * 0.9f + 0.1f }, rand() / (RAND_MAX + 1.0f));
                    break;

                case 2:
                    mat = Material(MaterialType::Glass, { rand() / (RAND_MAX + 1.0f * 0.9f + 0.1f) , rand() / (RAND_MAX + 1.0f * 0.9f + 0.1f) , rand() / (RAND_MAX + 1.0f) * 0.9f + 0.1f }, 0, 1.5);
                    break;

                default:
                    break;
                }

                float size = (rand() / (RAND_MAX + 1.0)) * 0.1 + 0.07;
                float yPos = size;
                if (!(i == 0 && j == 0))
                    scene.AddObject(("Sphere " + std::to_string(scene.GetObjectCount() + 1)).c_str(), (Object*) new Sphere({i * 0.7f, yPos, j * 0.7f - 2}, size, mat));
            }
        }

        Material metal = Material(MaterialType::Glossy, { 0.8, 0.8, 0.8 }, 0);
        Material glass = Material(MaterialType::Glass, { 1.0, 1.0, 1.0 }, 0, 1.44);

        Material light = Material(MaterialType::Lambertian, { 1, 1, 1 }, 1, 1, 20);

        scene.AddObject("Big Metal Sphere", (Object*)new Sphere({ 1.6, 0.35, -2 }, 0.35, metal));
        scene.AddObject("Big Glass Sphere", (Object*)new Sphere({ 0, 0.375, -2 },  0.375, glass));

        scene.AddObject("Light", (Object*)new Sphere({0,   1.5, -2}, 0.3, light));

        Material cube1 = Material(MaterialType::Lambertian, { 0.5, 0.5, 1 }, 0);
        scene.AddObject("Cube 1", (Object*)new Mesh({ -0.46, 0.1, 0-0.2 }, "cube.obj", cube1));

        Material cube2mat = Material(MaterialType::Lambertian, { 0.5, 1, 0.5 }, 0);
        Object* cube2 = (Object*)new Mesh({ 0.46, 0.06, 0 - 0.25 }, "cube.obj", cube2mat);
        cube2->SetScale({ 0.6, 0.6, 0.6 });
        scene.AddObject("Cube 2", cube2);

        Material amogus = Material(MaterialType::Lambertian, { 0.8, 0, 0 }, 0);
        scene.AddObject("Amogus", (Object*)new Mesh({ -0.18, 0, 0.36 }, "amogus.obj", amogus));

        Camera camera = Camera(aspectRatio, 3, { 0,  0.13, 0.8 });

        //scene.SetCamera(camera);
        scene = Scene::LoadFromFile("scene.scene");

        //scene.SaveToFile("scene.scene");
    }

    void SetUpImGui() {
        // Set up taken from https://github.com/ocornut/imgui/blob/master/examples/example_sdl_sdlrenderer/main.cpp

        // Setup Dear ImGui context
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO(); (void)io;
        //io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
        //io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
        ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_DockingEnable;

        // Setup Dear ImGui style
        //ImGui::StyleColorsDark();
        //ImGui::StyleColorsClassic();

        // Setup Platform/Renderer backends
        ImGui_ImplSDL2_InitForSDLRenderer(GetWindow()->GetRawWindow(), GetRenderer()->GetRawRenderer());
        ImGui_ImplSDLRenderer_Init(GetRenderer()->GetRawRenderer());

        io.Fonts->AddFontFromFileTTF("Roboto-Regular.ttf", 15.0f);

        SetImGuiStyle(darkMode);
    }

    void SetImGuiStyle(bool darkMode) {
        ImGuiStyle& style = ImGui::GetStyle();

        style.FrameRounding = 3.0f;
        style.WindowBorderSize = 0;

        if (darkMode) {
            //style.Alpha = 1.0f;
            //style.DisabledAlpha = 0.6000000238418579f;
            //style.WindowPadding = ImVec2(8.0f, 8.0f);
            //style.WindowRounding = 0.0f;
            //style.WindowBorderSize = 1.0f;
            //style.WindowMinSize = ImVec2(32.0f, 32.0f);
            //style.WindowTitleAlign = ImVec2(0.0f, 0.5f);
            //style.WindowMenuButtonPosition = ImGuiDir_Left;
            //style.ChildRounding = 0.0f;
            //style.ChildBorderSize = 1.0f;
            //style.PopupRounding = 0.0f;
            //style.PopupBorderSize = 1.0f;
            //style.FramePadding = ImVec2(4.0f, 3.0f);
            //style.FrameRounding = 0.0f;
            //style.FrameBorderSize = 0.0f;
            //style.ItemSpacing = ImVec2(8.0f, 4.0f);
            //style.ItemInnerSpacing = ImVec2(4.0f, 4.0f);
            //style.CellPadding = ImVec2(4.0f, 2.0f);
            //style.IndentSpacing = 21.0f;
            //style.ColumnsMinSpacing = 6.0f;
            //style.ScrollbarSize = 14.0f;
            //style.ScrollbarRounding = 0.0f;
            //style.GrabMinSize = 10.0f;
            //style.GrabRounding = 0.0f;
            //style.TabRounding = 0.0f;
            //style.TabBorderSize = 0.0f;
            //style.TabMinWidthForCloseButton = 0.0f;
            //style.ColorButtonPosition = ImGuiDir_Right;
            //style.ButtonTextAlign = ImVec2(0.5f, 0.5f);
            //style.SelectableTextAlign = ImVec2(0.0f, 0.0f);

            style.Colors[ImGuiCol_Text] = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
            style.Colors[ImGuiCol_TextDisabled] = ImVec4(0.5921568870544434f, 0.5921568870544434f, 0.5921568870544434f, 1.0f);
            style.Colors[ImGuiCol_WindowBg] = ImVec4(0.1725211292505264f, 0.172521248459816f, 0.1802574992179871f, 1.0f);
            style.Colors[ImGuiCol_ChildBg] = ImVec4(0.1450980454683304f, 0.1450980454683304f, 0.1490196138620377f, 1.0f);
            style.Colors[ImGuiCol_PopupBg] = ImVec4(0.1450980454683304f, 0.1450980454683304f, 0.1490196138620377f, 1.0f);
            style.Colors[ImGuiCol_Border] = ImVec4(0.1450980454683304f, 0.1450980454683304f, 0.1490196138620377f, 1.0f);
            style.Colors[ImGuiCol_BorderShadow] = ImVec4(0.3058823645114899f, 0.3058823645114899f, 0.3058823645114899f, 1.0f);
            style.Colors[ImGuiCol_FrameBg] = ImVec4(0.2000000029802322f, 0.2000000029802322f, 0.2156862765550613f, 1.0f);
            style.Colors[ImGuiCol_FrameBgHovered] = ImVec4(0.1137254908680916f, 0.5921568870544434f, 0.9254902005195618f, 1.0f);
            style.Colors[ImGuiCol_FrameBgActive] = ImVec4(0.0f, 0.4666666686534882f, 0.7843137383460999f, 1.0f);
            style.Colors[ImGuiCol_TitleBg] = ImVec4(0.1450980454683304f, 0.1450980454683304f, 0.1490196138620377f, 1.0f);
            style.Colors[ImGuiCol_TitleBgActive] = ImVec4(0.1450980454683304f, 0.1450980454683304f, 0.1490196138620377f, 1.0f);
            style.Colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.1450980454683304f, 0.1450980454683304f, 0.1490196138620377f, 1.0f);
            style.Colors[ImGuiCol_MenuBarBg] = ImVec4(0.2117647081613541f, 0.2117647081613541f, 0.2117647081613541f, 1.0f);
            style.Colors[ImGuiCol_ScrollbarBg] = ImVec4(0.2000000029802322f, 0.2000000029802322f, 0.2156862765550613f, 1.0f);
            style.Colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.321568638086319f, 0.321568638086319f, 0.3333333432674408f, 1.0f);
            style.Colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.3529411852359772f, 0.3529411852359772f, 0.3725490272045135f, 1.0f);
            style.Colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.3529411852359772f, 0.3529411852359772f, 0.3725490272045135f, 1.0f);
            style.Colors[ImGuiCol_CheckMark] = ImVec4(0.0f, 0.4666666686534882f, 0.7843137383460999f, 1.0f);
            style.Colors[ImGuiCol_SliderGrab] = ImVec4(0.1137254908680916f, 0.5921568870544434f, 0.9254902005195618f, 1.0f);
            style.Colors[ImGuiCol_SliderGrabActive] = ImVec4(0.0f, 0.4666666686534882f, 0.7843137383460999f, 1.0f);
            style.Colors[ImGuiCol_Button] = ImVec4(0.2000000029802322f, 0.2000000029802322f, 0.2156862765550613f, 1.0f);
            style.Colors[ImGuiCol_ButtonHovered] = ImVec4(0.1137254908680916f, 0.5921568870544434f, 0.9254902005195618f, 1.0f);
            style.Colors[ImGuiCol_ButtonActive] = ImVec4(0.1137254908680916f, 0.5921568870544434f, 0.9254902005195618f, 1.0f);
            style.Colors[ImGuiCol_Header] = ImVec4(0.2000000029802322f, 0.2000000029802322f, 0.2156862765550613f, 1.0f);
            style.Colors[ImGuiCol_HeaderHovered] = ImVec4(0.1137254908680916f, 0.5921568870544434f, 0.9254902005195618f, 1.0f);
            style.Colors[ImGuiCol_HeaderActive] = ImVec4(0.0f, 0.4666666686534882f, 0.7843137383460999f, 1.0f);
            style.Colors[ImGuiCol_Separator] = ImVec4(0.1450980454683304f, 0.1450980454683304f, 0.1490196138620377f, 1.0f);
            style.Colors[ImGuiCol_SeparatorHovered] = ImVec4(0.3058823645114899f, 0.3058823645114899f, 0.3058823645114899f, 1.0f);
            style.Colors[ImGuiCol_SeparatorActive] = ImVec4(0.3058823645114899f, 0.3058823645114899f, 0.3058823645114899f, 1.0f);
            style.Colors[ImGuiCol_ResizeGrip] = ImVec4(0.1450980454683304f, 0.1450980454683304f, 0.1490196138620377f, 1.0f);
            style.Colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.2000000029802322f, 0.2000000029802322f, 0.2156862765550613f, 1.0f);
            style.Colors[ImGuiCol_ResizeGripActive] = ImVec4(0.321568638086319f, 0.321568638086319f, 0.3333333432674408f, 1.0f);
            style.Colors[ImGuiCol_Tab] = ImVec4(0.1450980454683304f, 0.1450980454683304f, 0.1490196138620377f, 1.0f);
            style.Colors[ImGuiCol_TabHovered] = ImVec4(0.1137254908680916f, 0.5921568870544434f, 0.9254902005195618f, 1.0f);
            style.Colors[ImGuiCol_TabActive] = ImVec4(0.1725490242242813f, 0.1725490242242813f, 0.1803921610116959f, 1.0f);
            style.Colors[ImGuiCol_TabUnfocused] = ImVec4(0.1450980454683304f, 0.1450980454683304f, 0.1490196138620377f, 1.0f);
            style.Colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.1725490242242813f, 0.1725490242242813f, 0.1803921610116959f, 1.0f);
            style.Colors[ImGuiCol_PlotLines] = ImVec4(0.0f, 0.4666666686534882f, 0.7843137383460999f, 1.0f);
            style.Colors[ImGuiCol_PlotLinesHovered] = ImVec4(0.1137254908680916f, 0.5921568870544434f, 0.9254902005195618f, 1.0f);
            style.Colors[ImGuiCol_PlotHistogram] = ImVec4(0.0f, 0.4666666686534882f, 0.7843137383460999f, 1.0f);
            style.Colors[ImGuiCol_PlotHistogramHovered] = ImVec4(0.1137254908680916f, 0.5921568870544434f, 0.9254902005195618f, 1.0f);
            style.Colors[ImGuiCol_TableHeaderBg] = ImVec4(0.1882352977991104f, 0.1882352977991104f, 0.2000000029802322f, 1.0f);
            style.Colors[ImGuiCol_TableBorderStrong] = ImVec4(0.3098039329051971f, 0.3098039329051971f, 0.3490196168422699f, 1.0f);
            style.Colors[ImGuiCol_TableBorderLight] = ImVec4(0.2274509817361832f, 0.2274509817361832f, 0.2470588237047195f, 1.0f);
            style.Colors[ImGuiCol_TableRowBg] = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
            style.Colors[ImGuiCol_TableRowBgAlt] = ImVec4(1.0f, 1.0f, 1.0f, 0.05999999865889549f);
            style.Colors[ImGuiCol_TextSelectedBg] = ImVec4(0.0f, 0.4666666686534882f, 0.7843137383460999f, 1.0f);
            style.Colors[ImGuiCol_DragDropTarget] = ImVec4(0.1450980454683304f, 0.1450980454683304f, 0.1490196138620377f, 1.0f);
            style.Colors[ImGuiCol_NavHighlight] = ImVec4(0.1450980454683304f, 0.1450980454683304f, 0.1490196138620377f, 1.0f);
            style.Colors[ImGuiCol_NavWindowingHighlight] = ImVec4(1.0f, 1.0f, 1.0f, 0.699999988079071f);
            style.Colors[ImGuiCol_NavWindowingDimBg] = ImVec4(0.800000011920929f, 0.800000011920929f, 0.800000011920929f, 0.2000000029802322f);
            style.Colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.1450980454683304f, 0.1450980454683304f, 0.1490196138620377f, 1.0f);
        }
        else {
            ImGui::StyleColorsLight();
        }

       //style.Colors[ImGuiCol_PopupBg] = style.Colors[ImGuiCol_TitleBg];

        //for (int i = 0; i <= ImGuiCol_COUNT; i++) {
        //    ImGuiCol_ colourID = (ImGuiCol_)i;
        //    ImVec4& colour = style.Colors[i];
        //    // Make window and title bar background lighter.
        //    if (darkMode && (colourID == ImGuiCol_WindowBg || colourID == ImGuiCol_TitleBg)) {
        //        colour.x = 3 * colour.x;
        //        colour.y = 3 * colour.y;
        //        colour.z = 3 * colour.z;
        //    }
        //    // Make backgrounds darker. If statement condition taken from https://gist.github.com/dougbinks/8089b4bbaccaaf6fa204236978d165a9.
        //    if (!darkMode && (colourID != ImGuiCol_ModalWindowDimBg) &&
        //        (colourID != ImGuiCol_NavWindowingDimBg) &&
        //        ((colourID == ImGuiCol_FrameBg) ||
        //            (colourID == ImGuiCol_WindowBg) ||
        //            (colourID == ImGuiCol_ChildBg))) {
        //        colour.x = 0.9 * colour.x;
        //        colour.y = 0.9 * colour.y;
        //        colour.z = 0.9 * colour.z;
        //    }
        //}
    }

    bool OpenFile(std::string fileterList, std::string& filePath) {
        nfdchar_t* path = NULL;
        nfdresult_t result = NFD_OpenDialog(fileterList.c_str(), NULL, &path);
        if (result == nfdresult_t::NFD_ERROR) {
            Error(std::string("Failed to load file! ") + NFD_GetError());
            return false;
        }
        if (result == nfdresult_t::NFD_CANCEL) return false;

        filePath = path;
        return true;
    }

    bool SaveAs(std::string fileterList, std::string& filePath) {
        nfdchar_t* path = NULL;
        nfdresult_t result = NFD_SaveDialog(fileterList.c_str(), NULL, &path);
        if (result == nfdresult_t::NFD_ERROR) {
            Error(std::string("Failed to save file! ") + NFD_GetError());
            return false;
        }
        if (result == nfdresult_t::NFD_CANCEL) return false;

        filePath = path;
        return true;
    }

    std::string AbsolutePathToRelative(std::string path) {
        std::filesystem::path currentDir("./");
        std::filesystem::path absolute(path);
        return std::filesystem::relative(absolute, currentDir).generic_string();
    }

    void New() {
        redraw = true;
        scene = Scene();
    }

    void Save() {
        if (scene.GetName() != "") scene.Save();
        else SaveSceneAs();
    }

    void SaveSceneAs() {
        std::string path;
        if (SaveAs("scene", path)) {
            scene.SaveToFile((path + std::string(".scene")).c_str());
        }
    }

    void OpenScene() {
        std::string path;
        if (OpenFile("scene", path)) {
            path = AbsolutePathToRelative(path);
            scene = scene.LoadFromFile(path.c_str());
            redraw = true;
            loadedNewFile = true;
        }
    }

    void TraceVisRay(const Ray& ray, Line3D currentLine, float length, int depth, std::mt19937& rnd) {
        // Don't go on forever!
        if (depth >= visSettings.maxDepth) {
            //return Vector3f();
            //currentLine.end = ray.GetPointAt(10);
            //currentLine.endColour = currentLine.startColour - Vector3f(255, 0, 0) * 10 / 100;
            //lines.push_back(currentLine);
            //currentLine.start = currentLine.end;
            //currentLine.startColour = currentLine.endColour;

            return;
        }
        depth++;

        if (length > visSettings.maxDistance) return;

        RayPayload payload;
        if (scene.ClosestHit(ray, 0.001, FLT_MAX, payload)) {
            if (payload.object->show == false) return;

            Ray newRay = Ray(Vector3f(), Vector3f());
            float pdf;
            payload.material->Scatter(ray, newRay, pdf, payload, rnd);

            if (length + payload.t > visSettings.maxDistance) {
                currentLine.end = ray.GetPointAt(visSettings.maxDistance - length);
                length = visSettings.maxDistance;
            }
            else {
                length += payload.t;
                currentLine.end = payload.point;
            }
            currentLine.endAlpha = 1.0- (length / visSettings.maxDistance);

            lines.push_back(currentLine);
            currentLine.start = newRay.GetOrigin();
            currentLine.startAlpha = currentLine.endAlpha;

            if (payload.object->material.emitted != 0) return;

            TraceVisRay(newRay, currentLine, length, depth, rnd);
        }
        else {

            if (depth > 1) {
                currentLine.end = ray.GetPointAt(visSettings.maxDistance - length);
                length = visSettings.maxDistance;
                currentLine.endAlpha = 0;

                //currentLine.startColour = currentLine.endColour;
                lines.push_back(currentLine);
            }

            return;
        }
        //return { 0, 0, 0 };
        return;
    }

    void GenerateLines() {
        lines.clear();
        Vector3f origin = Vector3f(0, 0.375, -2);
        int divisions = visSettings.initialRays;
        std::vector<Ray> rays;
        Matrix4x4f rotation;
        Vector3f initialDirection = { 0, -1, 0 };
        //for (int i = 0; i < 360; i += 360/divisions) {
        //    for (int j = 0; j < 360; j += 360 / divisions) {
        //        for (int k = 0; k < 360; k += 360 / divisions) {
        //            rotation = Matrix4x4f::Rotate({ (float)i, (float)j, (float)k });
        //            Vector3f direction = rotation * initialDirection;
        //            direction.Normalize();
        //            bool alreadyAdded = false;
        //            for each (Ray ray in rays) {
        //                if (ray.GetDirection() == direction) alreadyAdded = true;
        //            }
        //            if (!alreadyAdded) rays.push_back({ origin, direction });
        //        }
        //    }
        //}

        for (float i = 0; i < 2 * M_PI; i += 2 * M_PI / divisions) {
            for (float j = 0; j < 2 * M_PI; j += 2 * M_PI / divisions) {
                Vector3f direction = Vector3f(sin(i) * cos(j), sin(j) * cos(i), cos(i));
                direction.Normalize();
                rays.push_back({ origin, direction });
                //lines.push_back({ origin, direction * 2 + origin });
            }
        }

        std::random_device r;
        std::seed_seq seed{ r(), r(), r(), r(), r(), r(), r(), r() };
        std::mt19937 rnd(seed);
        std::uniform_real_distribution<float> dist(0.0, 1.0);

        for each (Ray ray in rays) {
            Line3D line;
            line.start = origin;
            line.startAlpha = 1;
            TraceVisRay(ray, line, 0, 0, rnd);
        }
    }

    bool IsLineVisible(float depth, const Ray& toLine, const Line3D& line) {
        //Vector3f toLineEnd = toLine.GetPointAt(depth);
        //float R21 = (toLineEnd - toLine.GetOrigin()).Dot(toLineEnd - toLine.GetOrigin());
        //float R22 = (line.end - line.start).Dot(line.end - line.start);
        //float D4321 = (line.end - line.start).Dot(toLineEnd - toLine.GetOrigin());
        //float D3121 = (line.start - toLine.GetOrigin()).Dot(toLineEnd - toLine.GetOrigin());
        //float D4331 = (line.end - line.start).Dot(line.start - toLine.GetOrigin());
        //float D4332 = (line.end - line.start).Dot(line.start - toLineEnd);
        //float s = (D4321 * D4331 + D3121 * R22) / (R21 * R22 + D4321 * D4321);
        //float t = (D4321 * D3121 - D3121 * R21) / (R21 * R22 + D4321 * D4321);
        ////Vector3f closestPointOnLine = line.start + t * (line.end - line.start);
        //Vector3f closestPointOnLine = toLine.GetOrigin() + s * (toLineEnd - toLine.GetOrigin());
        //return (closestPointOnLine - scene.camera.position).Magnitude() < depth && s >= 0 && s <= 1;
        ////return s >= 0 && s <= 1;
        ////float closestOnLineToDepth = -D4332 / R22;
        ////Vector3f closestPos = line.start + closestOnLineToDepth * (line.end - line.start);
        ////return (closestPos - scene.camera.position).Magnitude() < depth;

        Vector3f posDiff = line.start - toLine.GetOrigin();
        Vector3f crossNormal = toLine.GetDirection().Cross(line.end - line.start);
        crossNormal.Normalize();
        Vector3f rejection = posDiff - toLine.GetDirection().Dot(posDiff) * toLine.GetDirection() - crossNormal.Dot(posDiff) * crossNormal;
        rejection.Normalize();
        Vector3f closestApproach = line.start - (line.end - line.start) * rejection / (line.end - line.start).Dot(rejection);
        printf("%f %f\n", (closestApproach - scene.camera.position).Magnitude(), depth);
        return (closestApproach - scene.camera.position).Magnitude() <= depth;

        //Vector3f d2 = line.end - line.start;
        //d2.Normalize();
        //Vector3f n = toLine.GetDirection().Dot(d2);
        //n.Normalize();
        //Vector3f n2 = (d2).Cross(n);
        //n2.Normalize();
        //Vector3f c1 = toLine.GetOrigin() + ((line.start - toLine.GetOrigin()).Dot(n2) / (toLine.GetDirection().Dot(n2))) * toLine.GetDirection();
        //return (c1 - scene.camera.position).Magnitude() < depth;
    }

    void UpdateImGui() {
        ImGui_ImplSDLRenderer_NewFrame();
        ImGui_ImplSDL2_NewFrame();
        ImGui::NewFrame();

        ImGui::DockSpaceOverViewport();
        ImGui::ShowDemoWindow();

        // Menu bar.
        {
            if (ImGui::BeginMainMenuBar()) {
                if (ImGui::BeginMenu("File")) {
                    if (ImGui::MenuItem("New")) {
                        New();
                    }
                    if (ImGui::MenuItem("Save")) {
                        Save();
                    }
                    if (ImGui::MenuItem("Save As")) {
                        SaveSceneAs();
                    }
                    if (ImGui::MenuItem("Open")) {
                        OpenScene();
                    }
                    ImGui::EndMenu();
                }

                if (ImGui::BeginMenu("Interface")) {
                    if (ImGui::Checkbox("Dark Mode", &darkMode)) {
                        SetImGuiStyle(darkMode);
                    }
                    ImGui::EndMenu();
                }

                ImGui::EndMainMenuBar();
            }
        }

        // FPS overlay
        {
            ImGui::SetNextWindowBgAlpha(0.9f);
            ImGui::Begin("Overlay", (bool*)1, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoDocking);
            ImGui::Text("%.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
            ImGui::Text("Renderer: %.3f ms/frame (%.1f FPS)", rendererFps * 1000, 1000/(rendererFps * 1000));

            ImGui::End();
        }

        // Camera position
        {
            ImGui::Begin("Camera");
            redraw |= ImGui::DragFloat3("Position", (float*)&scene.camera.position, 0.01);
            //ImGui::DragFloat3("Rotation", (float*)&obj->rotation, 0.5);

            ImGui::End();
        }

        // Object selection window.
        {
            ImGui::Begin("Objects");
            if (ImGui::Button("Add New Object", { ImGui::GetWindowWidth(), 20 })) {
                //scene.AddObject(GenerateRandomObject());
                ImGui::OpenPopup("Add Object");
            }

            ImGui::SetNextWindowPos(ImGui::GetWindowPos(), ImGuiCond_Always, ImVec2(1, 0));
            if (ImGui::BeginPopup("Add Object")) {

                char nameBuffer[100];
                sprintf_s(nameBuffer, newObjectName.c_str(), 100);
                ImGui::InputText("Name", nameBuffer, 100);
                newObjectName = nameBuffer;

                const char* objectTypes[] = { "Sphere", "Cube", "3D Model" };
                if (ImGui::BeginCombo("Material Type", objectTypes[newObjectType])) {
                    for (int i = 0; i < IM_ARRAYSIZE(objectTypes); i++) {
                        const bool selected = (newObjectType == i);
                        if (ImGui::Selectable(objectTypes[i], selected)) {
                            newObjectType = i;
                        }

                        if (selected)
                            ImGui::SetItemDefaultFocus();
                    }
                    ImGui::EndCombo();
                }

                bool valid = true;

                if (objectTypes[newObjectType] == "3D Model") {
                    char buffer[100];
                    sprintf_s(buffer, newObjectPath.c_str(), newObjectPath.length());
                    std::ifstream file(newObjectPath);
                    valid = file.good();
                    if (!valid) ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1, 0, 0, 1));
                    ImGui::InputText("File Path", buffer, 100);
                    if (!valid) ImGui::PopStyleColor();
                    newObjectPath = buffer;

                    if (ImGui::Button("Browse for file")) {
                        nfdchar_t* path = NULL;
                        nfdresult_t result = NFD_OpenDialog("obj", NULL, &path);
                        newObjectPath = AbsolutePathToRelative(path);
                    }
                }

                if (ImGui::Button("Ok") && valid) {
                    ImGui::CloseCurrentPopup();
                    Object* object;

                    if (objectTypes[newObjectType] == "Sphere") {
                        object = (Object*) new Sphere(Vector3f(), 0.5, Material());
                    }
                    else if (objectTypes[newObjectType] == "Cube") {
                        object = (Object*) new Mesh({ 0, 0, 0 }, "Cube.obj", Material());
                    }
                    else if (objectTypes[newObjectType] == "3D Model") {
                        object = (Object*) new Mesh({ 0, 0, 0 }, newObjectPath.c_str(), Material());
                    }

                    redraw = true;
                    scene.AddObject(newObjectName.c_str(), object);
                    selectedObject = scene.GetObjectCount() - 1;
                    newObjectName = "Object " + std::to_string(scene.GetObjectCount());
                    newObjectPath = "";
                    newObjectType = 0;
                }
                if (!valid && ImGui::IsItemHovered()) ImGui::SetMouseCursor(ImGuiMouseCursor_NotAllowed);

                ImGui::SameLine();
                if (ImGui::Button("Close")) ImGui::CloseCurrentPopup();

                ImGui::EndPopup();
            }

            for (int n = 0; n < scene.GetObjectCount(); n++) {
                ImGui::SetItemAllowOverlap();

                ImGui::SetCursorPos(ImVec2(8, n * 25 + 20 + 35));

                char buf[32];
                sprintf(buf, "##Show %d", n + 1); // Hidden id for the checkbox.
                Object* obj = scene.GetObjects()[n];

                // Checkbox to show and hide object.
                redraw |= ImGui::Checkbox(buf, &obj->show);

                ImGui::SetCursorPos(ImVec2(35, n * 25 + 20 + 3 + 35));
                sprintf(buf, (std::string("##object_id_") + std::to_string(scene.GetObjectID(obj))).c_str(), n + 1);
                if (ImGui::Selectable(buf, selectedObject == n))
                    selectedObject = n; // Set currently selected object.
                ImGui::SameLine();
                ImGui::Text(obj->name.c_str());
            }

            ImGui::End();
        }

        // Pannel to edit currently selected object.
        {
            ImGui::Begin("Object Properties");

            if (selectedObject >= 0) {
                Object* obj = scene.GetObjects()[selectedObject];

                char buffer[32];
                sprintf_s(buffer, obj->name.c_str(), obj->name.length());

                ImGui::InputText("Name", buffer, 32);

                Vector3f position = obj->GetPosition();
                Vector3f rotation = obj->GetRotation();
                Vector3f scale = obj->GetScale();

                if (ImGui::DragFloat3("Position", (float*)&position, 0.01)) {
                    redraw = true;
                    obj->SetPosition(position);
                }

                if (ImGui::DragFloat3("Rotation", (float*)&rotation, 1)) {
                    redraw = true;
                    // TODO: Check!
                    rotation.x = fmod(rotation.x, 360);
                    rotation.y = fmod(rotation.y, 360);
                    rotation.z = fmod(rotation.z, 360);
                    obj->SetRotation(rotation);
                }

                if (ImGui::DragFloat3("Scale", (float*)&scale, 0.01)) {
                    redraw = true;
                    obj->SetScale(scale);
                }

                obj->name = std::string(buffer);

                if (ImGui::Button("Delete")) {
                    scene.RemoveObject(obj);
                    selectedObject--;
                    redraw = true;
                }
            }
            else {
                ImGui::Text("Select an object to edit it's properties.");
            }
            ImGui::End();
        }

        // Pannel to edit the material of the currently selected object.
        {
            ImGui::Begin("Material Editor");

            if (selectedObject >= 0) {
                Object* obj = scene.GetObjects()[selectedObject];
                Material* material = &obj->material;

                const char* materials[] = { "Lambertian", "Specular", "Glass" };

                const char* comboText = materials[(int)material->materialType];  // Pass in the preview value visible before opening the combo (it could be anything)
                if (ImGui::BeginCombo("Material Type", comboText)) {
                    for (int i = 0; i < IM_ARRAYSIZE(materials); i++) {
                        const bool selected = ((int)material->materialType == i);
                        if (ImGui::Selectable(materials[i], selected)) {
                            material->materialType = (MaterialType)i;
                            redraw = true;
                        }

                        if (selected)
                            ImGui::SetItemDefaultFocus();
                    }
                    ImGui::EndCombo();
                }

                redraw |= ImGui::ColorEdit3("Colour", (float*)&material->colour);
                if (material->materialType != MaterialType::Glass) redraw |= ImGui::DragFloat("Emitted", (float*)&material->emitted, 0.1, 0, 100);
                //if (material->type != MaterialType::Glass) ImGui::SliderFloat("Albedo", &material->albedo, 0, 1);
                if (material->materialType == MaterialType::Glass) redraw |= ImGui::SliderFloat("Index of Refraction", &material->refractiveIndex, 1, 2);
                if (material->materialType != MaterialType::Lambertian) redraw |= ImGui::SliderFloat("Roughness", &material->roughness, 0, 1000);
            }
            else {
                ImGui::Text("Select an object to edit it's material.");
            }
            ImGui::End();
        }

        // Pannel to set the renderer configuration.
        {
            ImGui::Begin("Render Settings");
            if (ImGui::BeginTabBar("Render Settings Tab Bar")) {
                if (ImGui::BeginTabItem("Preview")) {
                    redraw |= ImGui::SliderInt("Max Depth", &renderSettings.maxDepth, 1, 100);
                    redraw |= ImGui::SliderInt("Samples Per Frame", &renderSettings.samples, 1, 100);
                    redraw |= ImGui::ColorEdit3("Ambient Light Colour", (float*)&renderSettings.ambientLight);
                    redraw |= ImGui::Checkbox("Checkerboard", &renderSettings.checkerboard);
                    redraw |= ImGui::Checkbox("Explicit Light Sampling", &renderSettings.directLightSampling);
                    ImGui::EndTabItem();
                }
                //if (ImGui::BeginTabItem("Output")) {
                //    ImGui::SliderInt("Max Depth", &outputSettings.depth, 1, 100);
                //    ImGui::SliderInt("Samples Per Frame", &outputSettings.samples, 1, 100);
                //    ImGui::ColorEdit3("Ambient Light Colour", (float*)&outputSettings.ambient);
                //    ImGui::EndTabItem();
                //}
                ImGui::EndTabBar();
            }

            ImGui::End();
        }

        //// Pannel to control the ray visualisation overlay.
        //{
        //    ImGui::Begin("Ray Illustration");
        //    ImGui::Checkbox("Enable", &enableRayLines);
        //    ImGui::SliderInt("Bounces per line", &rayLineDepth, 1, 10);
        //    ImGui::SliderInt("Lines per light source", &rayLinesPerLightSource, 1, 10);
        //    ImGui::ColorEdit3("Ray Line Colour", (float*)&rayLineColour);
        //    ImGui::End();
        //}

        // The window which contains the renderer output.
        {
            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
            ImGui::Begin("Scene");
            ImGui::SetCursorPos({ 0, 0 });

            ImGui::SetItemAllowOverlap();

            SDL_Point size;
            SDL_QueryTexture(finalImage->GetRawTexture(), NULL, NULL, &size.x, &size.y);

            float scale = (ImGui::GetWindowWidth()) / size.x;
            if (scale * size.y > ImGui::GetWindowHeight()) scale = (ImGui::GetWindowHeight()) / size.y;
            //scale = 1;
            //ImGui::SetCursorPos({ 0, 25 });
            ImVec2 imageStartScreen = ImGui::GetCursorScreenPos();
            //ImGui::GetWindowDrawList()->AddImage(
            //    (void*)finalImage->GetRawTexture(),
            //    ImVec2(ImGui::GetCursorScreenPos()),
            //    ImVec2((int)(ImGui::GetCursorScreenPos().x + size.x * scale), (int)(ImGui::GetCursorScreenPos().y + size.y * scale)));
            ImGui::Image((void*)(intptr_t)finalImage2, ImVec2(size.x * scale, size.y * scale));

            if (ImGui::IsWindowHovered() && ImGui::IsMouseClicked(0) && ImGui::IsMouseHoveringRect(imageStartScreen, { imageStartScreen.x + size.x * scale, imageStartScreen.y + size.y * scale })) {
                ImVec2 pos = ImGui::GetMousePos();
                Vector2f relativePos = { (pos.x - imageStartScreen.x) / (scale * size.x), (pos.y - imageStartScreen.y) / (scale * size.y) };
                Vector3f viewportPos = scene.camera.GetViewportPos(relativePos);
                Ray ray = Ray(scene.camera.position, viewportPos);
                RayPayload payload;
                if (scene.ClosestHit(ray, 0.001, FLT_MAX, payload)) {
                    selectedObject = scene.GetObjectID(payload.object);
                }
                else {
                    selectedObject = -1;
                }
            }
            //std::cout << ImGui::GetWindowWidth() << " " << std::round(size.x * scale) << std::endl;
            //if (std::round(size.y * scale) != ImGui::GetWindowHeight() || std::round(size.x * scale) != 1631) {
            //    float aspectRatio = ImGui::GetWindowWidth() / ImGui::GetWindowHeight();
            //    width = height * aspectRatio;
            //    renderSettings.resolution = { (float)width, (float)height };

            //    image = new Colour * [width];
            //    isSelectedObjectVisible = new bool* [width];
            //    for (int i = 0; i < width; i++) {
            //        image[i] = new Colour[height];
            //        isSelectedObjectVisible[i] = new bool[height];
            //    }

            //    finalImage = new Texture(width, height);
            //    finalImage2 = SDL_CreateTexture(Application::GetApp()->GetRenderer()->GetRawRenderer(), SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, width, height);

            //    SDL_SetTextureScaleMode(finalImage2, SDL_ScaleModeBest);

            //    scene.camera = Camera(aspectRatio, 2, scene.camera.position);
            //}

            ImGui::End();
            ImGui::PopStyleVar();
        }

        {
            ImGui::Begin("Ray Visualization");
            ImGui::Checkbox("Show visualization", &showVisualizualisation);
            ImGui::DragInt("Initial rays cast", &visSettings.initialRays, 1, 0, 100);
            ImGui::DragInt("Max depth", &visSettings.maxDepth, 1, 0, 100);
            ImGui::DragFloat("Max distance", &visSettings.maxDistance, 0.01, 0.01, 10);
            ImGui::ColorEdit3("Colour", (float*)&visSettings.lineColour);
            if (ImGui::Button("Generate Rays")) GenerateLines();
            ImGui::End();
        }

        ImGui::Render();
        ImGui_ImplSDLRenderer_RenderDrawData(ImGui::GetDrawData());

    }

    void DrawBresenhamLine(Texture* image, Line2D line, Line3D line3d) {
        image->Lock();
        float m = (line.end.y - line.start.y) / (line.end.x - line.start.x);
        float c = line.start.y - m * line.start.x;
        int xDirection = line.start.x < line.end.x ? 1 : -1;
        int yDirection = line.start.y < line.end.y ? 1 : -1;
        int xCoord = line.start.x;
        int yCoord = line.start.y;

        float alpha = line.startAlpha;
        Colour colour;

        if (abs(m) <= 1) {
            float alphaDiff = (line.endAlpha - line.startAlpha) / abs((line.end.x - line.start.x) + 1);
            float actualY;
            for (xCoord; xCoord != line.end.x; xCoord += xDirection) {
                actualY = m * xCoord + c;
                float d1 = abs(actualY - yCoord);
                float d2 = abs(yCoord + yDirection - actualY);
                if (d1 - d2 > 0)
                    yCoord += yDirection;

                if (alpha < 0) alpha = 0;
                if (alpha > 1) alpha = 1;

                if (xCoord >= 0 && xCoord < image->GetWidth() && yCoord >= 0 && yCoord < image->GetHeight()) {
                    Vector3f toLine = scene.camera.GetViewportPos({ (float)xCoord, (float)yCoord });
                    toLine.Normalize();
                    if (IsLineVisible(depthMap[xCoord][yCoord], Ray(scene.camera.position, toLine), line3d)) {
                        colour = line.colour * 255 * alpha + image->GetColourAt({ (float)xCoord, (float)yCoord }) * (1 - alpha);
                        image->SetColourAt({ (float)xCoord, (float)yCoord }, colour);
                    }
                }

                alpha += alphaDiff;
            }
        }
        else {
            float actualX;
            float alphaDiff = (line.endAlpha - line.startAlpha) / abs((line.end.y - line.start.y) + 1);
            for (yCoord; yCoord != line.end.y; yCoord += yDirection) {
                actualX = (yCoord - c) / m;
                float d1 = abs(actualX - xCoord);
                float d2 = abs(xCoord + xDirection - actualX);
                if (d1 - d2 > 0)
                    xCoord += xDirection;

                if (alpha < 0) alpha = 0;
                if (alpha > 1) alpha = 1;


                if (xCoord >= 0 && xCoord < image->GetWidth() && yCoord >= 0 && yCoord < image->GetHeight()) {
                    Vector3f toLine = scene.camera.GetViewportPos({ (float)xCoord, (float)yCoord });
                    toLine.Normalize();
                    if (IsLineVisible(depthMap[xCoord][yCoord], Ray(scene.camera.position, toLine), line3d)) {
                        colour = line.colour * 255 * alpha + image->GetColourAt({ (float)xCoord, (float)yCoord }) * (1 - alpha);
                        image->SetColourAt({ (float)xCoord, (float)yCoord }, colour);
                    }
                }

                alpha += alphaDiff;
            }
        }

        image->Unlock();
    }

    void Update(float deltaTime) {
        std::string title = "Ray Tracing Optics Simulator: ";
        if (scene.GetName() != "") title += scene.GetName();
        else title += "unnamed";
        if (scene.IsModified()) title += "*";
        GetWindow()->SetTitle(title.c_str());

        if (redraw && !loadedNewFile) scene.SetModified();
        loadedNewFile = false;

        // Start rendering again from scratch if settings are changed.
        if (redraw) frame = 1;
        redraw = false;
        frame++;

        std::chrono::system_clock::time_point previousTime = std::chrono::system_clock::now();
        image = GetRenderer()->RenderScene(scene, image, depthMap, renderSettings, frame);
        std::chrono::system_clock::time_point currentTime = std::chrono::system_clock::now();
        std::chrono::duration<float> elapsed = std::chrono::duration_cast<std::chrono::duration<float>>(currentTime - previousTime);
        rendererFps = elapsed.count();

        for (int y = 0; y < renderSettings.resolution.y; y++) {
            for (int x = 0; x < renderSettings.resolution.x; x++) {
                if (selectedObject >= 0) {
                    Vector2f screenPos = { x / renderSettings.resolution.x, y / renderSettings.resolution.y };
                    Vector3f viewportPos = scene.camera.GetViewportPos(screenPos);
                    Ray ray = Ray(scene.camera.position, viewportPos);
                    Object* selected = scene.GetObjects()[selectedObject];
                    RayPayload payload;
                    isSelectedObjectVisible[x][y] = selected->Intersect(ray, 0, FLT_MAX, payload);
                }
                else {
                    isSelectedObjectVisible[x][y] = false;
                }
            }
        }

        //Line3D line3d = { Vector3f(-0.2, 0, 0.36), Vector3f(0.43, 0.23, -0.25) };
        //Line2D line;
        //line.start = scene.camera.GetScreenPos(line3d.start) * renderSettings.resolution;
        //line.end = scene.camera.GetScreenPos(line3d.end) * renderSettings.resolution;

       // Prepare image texture for rendering.
        finalImage->Lock();
        for (int y = 0; y < renderSettings.resolution.y; y++) {
            for (int x = 0; x < renderSettings.resolution.x; x++) {
                Colour colour = Vector3f();

                if (renderSettings.checkerboard && !((x % 2 == 0 && y % 2 == 0) || (x % 2 != 0 && y % 2 != 0))) {
                    int samples = 0;
                    if (x - 1 >= 0) {
                        colour += image[x - 1][y];
                        samples++;
                    }
                    if (x + 1 < renderSettings.resolution.x) {
                        colour += image[x + 1][y];
                        samples++;
                    }
                    if (y - 1 >= 0) {
                        colour += image[x][y - 1];
                        samples++;
                    }
                    if (y + 1 < renderSettings.resolution.y) {
                        colour += image[x][y + 1];
                        samples++;
                    }

                    colour /= samples;
                }
                else {
                    colour = image[x][y];
                }

                finalImage->SetColourAt({ (float)x, (float)y }, colour * 255);
            }
        }


        for (int y = 0; y < renderSettings.resolution.y; y++) {
            for (int x = 0; x < renderSettings.resolution.x; x++) {

                //Vector2f lineDirection = line.end - line.start;
                //Vector2f toPixel = Vector2f(x, y) - line.start;
                //lineDirection.Normalize();
                //toPixel.Normalize();
                //if (toPixel.Dot(lineDirection) > 0.99999) finalImage->SetColourAt({ (float)x, (float)y }, { 255, 0, 100 });

                //for each (Line2D line in lines2d) {
                //    float grad = (line.end.y - line.start.y) / (line.end.x - line.start.x);
                //    if (abs(y - line.start.y - grad * (x - line.start.x)) < 1)
                //        //if (x >= line.start.x && x <= line.end.x && y >= line.start.y && y <= line.end.x)
                //        finalImage->SetColourAt({ (float)x, (float)y }, line.startColour);
                //}

                if (isSelectedObjectVisible[x][y]) {
                    for (int xOffset = x - 1; xOffset <= x + 1; xOffset++) {
                        for (int yOffset = y - 1; yOffset <= y + 1; yOffset++) {
                            if (xOffset >= 0 && xOffset < renderSettings.resolution.x && yOffset >= 0 && yOffset < renderSettings.resolution.y) {
                                if (!isSelectedObjectVisible[xOffset][yOffset]) {
                                    finalImage->SetColourAt({ (float)xOffset, (float)yOffset }, { 0, 0, 0 });
                                }
                            }
                        }
                    }
                }
            }
        }
        finalImage->Unlock();

        //GenerateLines();
        if (showVisualizualisation) {
            for each (Line3D line in lines) {
                if ((line.start - scene.camera.position).Dot(Vector3f(0, 0, -1)) <= 0 || (line.end - scene.camera.position).Dot(Vector3f(0, 0, -1)) <= 0) continue;
                Line2D line2d = { scene.camera.GetScreenPos(line.start) * renderSettings.resolution, scene.camera.GetScreenPos(line.end) * renderSettings.resolution, visSettings.lineColour, line.startAlpha, line.endAlpha };
                line2d.start.x = (int)line2d.start.x;
                line2d.start.y = (int)line2d.start.y;
                line2d.end.x = (int)line2d.end.x;
                line2d.end.y = (int)line2d.end.y;
                DrawBresenhamLine(finalImage, line2d, line);
            }
        }

        //DrawBresenhamLine(finalImage, { Vector2f(10, 200), Vector2f(300, 10), {0, 255, 0}, 0, 1 });

        SDL_SetRenderTarget(GetRenderer()->GetRawRenderer(), finalImage2);
        SDL_RenderCopy(GetRenderer()->GetRawRenderer(), finalImage->GetRawTexture(), NULL, NULL);
        //SDL_RenderDrawLine(GetRenderer()->GetRawRenderer(), line.start.x, line.start.y, line.end.x, line.end.y);

        //if (showVisualizualisation) {
        //    for each (Line2D line in lines2d) {
        //        SDL_SetRenderDrawColor(GetRenderer()->GetRawRenderer(), visSettings.lineColour.x * 255, visSettings.lineColour.y * 255, visSettings.lineColour.z * 255, line.endAlpha * 255);
        //        SDL_RenderDrawLine(GetRenderer()->GetRawRenderer(), line.start.x, line.start.y, line.end.x, line.end.y);
        //    }
        //}
                //SDL_SetRenderDrawColor(GetRenderer()->GetRawRenderer(), 255, 255, 0, 155);
                //SDL_RenderDrawLine(GetRenderer()->GetRawRenderer(), 10, 200, 200, 10);

        SDL_SetRenderTarget(GetRenderer()->GetRawRenderer(), NULL);

        // Process input for ImGui.
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            ImGui_ImplSDL2_ProcessEvent(&event);
            if (event.type == SDL_QUIT) {
                mIsRunning = false;
            }
            if (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_CLOSE && event.window.windowID == SDL_GetWindowID(GetWindow()->GetRawWindow())) {
                mIsRunning = false;
            }
        }

        const Uint8* keyboardState = SDL_GetKeyboardState(NULL);
        if (keyboardState[SDL_SCANCODE_LCTRL] && keyboardState[SDL_SCANCODE_LSHIFT] && keyboardState[SDL_SCANCODE_S]) {
            SaveSceneAs();
        }
        else if (keyboardState[SDL_SCANCODE_LCTRL] && keyboardState[SDL_SCANCODE_S]) {
            Save();
        }
        else if (keyboardState[SDL_SCANCODE_LCTRL] && keyboardState[SDL_SCANCODE_O]) {
            OpenScene();
        }
        else if (keyboardState[SDL_SCANCODE_LCTRL] && keyboardState[SDL_SCANCODE_N]) {
            New();
        }

        // Draw the interface.
        GetRenderer()->Clear();
        UpdateImGui();
    }

private:
    Colour** image;       // The image generated by the renderer.
    bool** isSelectedObjectVisible;
    float** depthMap;
    Texture* finalImage;  // The image which will be drawn.
    SDL_Texture* finalImage2;  // The image which will be drawn.

    RenderSettings renderSettings;

    Scene scene;

    int width;
    int height;
    float aspectRatio;

    int frame = 0;        // Frame referes to the number of frames that nothing has changed, used for temporal anti-aliasing.

    // UI state:
    std::string newObjectName;
    int newObjectType;
    std::string newObjectPath;
    int selectedObject;

    bool redraw;
    bool loadedNewFile;
    ImVec4 ambientLightColour;

    std::vector<Line3D> lines;
    RayVisualizationSettings visSettings;

    float rendererFps;

    bool darkMode;
    bool showVisualizualisation;
};

int main(int argc, char* argv[]) {

    Application* app = new PrototypeApp(argv[1]);
    app->Run();

    return 0;
}
