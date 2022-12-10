#include "imgui.h"
#include "imgui_impl_sdl.h"
#include "imgui_impl_sdlrenderer.h"
#include <stdio.h>
#include <iostream>
#include <SDL.h>
#include <nfd.h>
#include <nlohmann/json.hpp>

#include <fstream>
#include <filesystem>
#include <string>
#include <thread>
#include <vector>
#include <algorithm>

#include <Application/Application.h>
#include <Maths/Ray.h>
#include <Maths/Vector3f.h>
#include <Renderer/Texture.h>
#include <Renderer/Camera.h>
#include <Renderer/Sphere.h>
#include <Renderer/DivergingLens.h>
#include <Renderer/Mesh.h>
#include <Renderer/Line.h>

#include <Error.h>

using namespace Prototype;
using json = nlohmann::json;

struct RayVisualizationSettings {
    bool      enable             = false;
    bool      updateRegularly    = true;
    int       framesPerUpdate    = 2;
    int       maxBounceDepth     = 4;
    int       initialRays        = 8;
    bool      shortenRays        = true;
    float     shortenedRayLength = 0.3;
    bool      ignoreSky          = true;
    bool      ignoreGround       = false;
    float     maxDistance        = 1.5;
    bool      sameColour         = false;
    Colour    lineColour         = { 1, 0, 0 };

    void LoadFromJSON(json data) {
        enable                   = data["enable"];
        updateRegularly          = data["enable"];
        framesPerUpdate          = data["enable"];
        maxBounceDepth           = data["enable"];
        initialRays              = data["enable"];
        shortenRays              = data["enable"];
        shortenedRayLength       = data["enable"];
        ignoreSky                = data["enable"];
        ignoreGround = data["enable"];
        maxDistance = data["enable"];
        sameColour = data["enable"];
        //lineColour = data["enable"];
    }
};

class FinalApp : public Application {
public:
    FinalApp(const char* file) : Application("Ray Tracing Optics Simulator", 1900 * 0.75, 1080 * 0.75) {
        mDarkMode = true;

        // Initialize UI
        SetUpImGui();

        // Internal resolution
        mInternalWidth = 450 * 1;
        mInternalHeight = 225 * 1;
        mAspectRatio = (float)mInternalWidth / (float)mInternalHeight;

        // Need to use an array as texture data wraps around after 255!
        mRenderedImage = new Colour * [mInternalWidth];
        mIsSelectedObjectVisible = new bool * [mInternalWidth];
        mDepthBuffer = new float * [mInternalWidth];
        mDepthBufferOld = new float * [mInternalWidth];
        for (int i = 0; i < mInternalWidth; i++) {
            mRenderedImage[i] = new Colour[mInternalHeight];
            mIsSelectedObjectVisible[i] = new bool[mInternalHeight];
            mDepthBuffer[i] = new float[mInternalHeight];
            mDepthBufferOld[i] = new float[mInternalHeight];
        }

        // Default renderer settings
        mPreviewRenderSettings = RenderSettings();
        mPreviewRenderSettings.resolution = { (float)mInternalWidth, (float)mInternalHeight };
        mPreviewRenderSettings.maxDepth = 10;
        mPreviewRenderSettings.samples = 1;
        mPreviewRenderSettings.ambientLight = Vector3f(1, 1, 1);
        mPreviewRenderSettings.checkerboard = false;
        mPreviewRenderSettings.directLightSampling = true;

        mRedrawThisFrame = false;
        mLoadedSceneThisFrame = true;

        // The image which will be rendered on the GUI.
        mOutputTexture = new Texture(mInternalWidth, mInternalHeight);
        SDL_SetTextureScaleMode(mOutputTexture->GetRawTexture(), SDL_ScaleModeBest); // TODO: Put in Texture class

        if (!file) mScene = Scene::LoadFromFile("scene.scene");
        else mScene = mScene = Scene::LoadFromFile(file);

        mSelectedObject = -1;

        mNewObjectType = 0;
        mNewObjectPath = "";
        mNewObjectName = "Object " + std::to_string(mScene.GetObjectCount());

        GenerateVisualization();
    }

    void SetUpImGui() {
        // Set up taken from https://github.com/ocornut/imgui/blob/master/examples/example_sdl_sdlrenderer/main.cpp

        // Setup Dear ImGui context
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO(); (void)io;
        ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_DockingEnable;

        // Setup Platform/Renderer backends
        ImGui_ImplSDL2_InitForSDLRenderer(GetWindow()->GetRawWindow(), GetRenderer()->GetRawRenderer());
        ImGui_ImplSDLRenderer_Init(GetRenderer()->GetRawRenderer());

        io.Fonts->AddFontFromFileTTF("Roboto-Regular.ttf", 15.0f);

        SetImGuiStyle(mDarkMode);
    }

    void SetImGuiStyle(bool darkMode) {
        ImGuiStyle& style = ImGui::GetStyle();

        style.FrameRounding = 3.0f;
        style.WindowBorderSize = 0;

        if (darkMode) {
            // Created in ImThemes by modifying the Visual Studio theme.
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
    }

    void LoadSettingsFromFile(std::string filePath) {
        std::ifstream file(filePath);
        json data = json::parse(file);

        

        file.close();
    }

    bool OpenFile(std::string fileterList, std::string& filePath) {
        nfdchar_t* path = NULL;
        nfdresult_t result = NFD_OpenDialog(fileterList.c_str(), NULL, &path);
        if (result == nfdresult_t::NFD_ERROR) {
            Error(std::string("NFD: Failed to load file! ") + NFD_GetError());
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
            Error(std::string("NFD: Failed to save file! ") + NFD_GetError());
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
        mRedrawThisFrame = true;
        mScene = Scene();

        mSelectedObject = -1;

        mNewObjectType = 0;
        mNewObjectPath = "";
        mNewObjectName = "Object " + std::to_string(mScene.GetObjectCount());

        GenerateVisualization();
    }

    void Save() {
        if (mScene.GetName() != "") mScene.Save();
        else SaveSceneAs();
    }

    void SaveSceneAs() {
        std::string path;
        if (SaveAs("scene", path)) {
            mScene.SaveToFile((path + std::string(".scene")).c_str());
        }
    }

    void OpenScene() {
        std::string path;
        if (OpenFile("scene", path)) {
            path = AbsolutePathToRelative(path);
            mScene = mScene.LoadFromFile(path.c_str());
            mRedrawThisFrame = true;
            mLoadedSceneThisFrame = true;
        }
    }

    void TraceVisRay(const Ray& ray, Line3D currentLine, float length, int depth, std::mt19937& rnd) {
        // Don't go on forever!
        if (depth >= mRayVisualizationSettings.maxBounceDepth) {
            return;
        }
        depth++;

        if (length > mRayVisualizationSettings.maxDistance) return;

        RayPayload payload;
        if (mScene.ClosestHit(ray, 0.001, FLT_MAX, payload)) {
            // Ignore the first object hit, since it is the object casting the light.
            if (depth == 1) {
                Ray rayCont = Ray(payload.point, ray.GetDirection());
                TraceVisRay(rayCont, currentLine, length, depth, rnd);
                return;
            }

            // Ignore rays which hit the ground
            if (payload.object->name == "Ground" && mRayVisualizationSettings.ignoreGround) return;

            Ray newRay = Ray(Vector3f(), Vector3f());
            float pdf;
            payload.material->Scatter(ray, newRay, pdf, payload, rnd);

            if (length + payload.t > mRayVisualizationSettings.maxDistance) {
                currentLine.end = ray.GetPointAt(mRayVisualizationSettings.maxDistance - length);
                length = mRayVisualizationSettings.maxDistance;
            }
            else {
                length += payload.t;
                currentLine.end = payload.point;
            }
            currentLine.endAlpha = 1.0- (length / mRayVisualizationSettings.maxDistance);

            mRayVisualizationLines.push_back(currentLine);
            currentLine.start = newRay.GetOrigin();
            currentLine.startAlpha = currentLine.endAlpha;

            // Continue recursively.
            TraceVisRay(newRay, currentLine, length, depth, rnd);
        }
        else {
            // Draw rays which go off into the sky.
            if (depth > 1 && !mRayVisualizationSettings.ignoreSky) {
                if (mRayVisualizationSettings.shortenRays) {
                    currentLine.end = ray.GetPointAt(mRayVisualizationSettings.shortenedRayLength);
                    length += mRayVisualizationSettings.shortenedRayLength;
                }
                else {
                    currentLine.end = ray.GetPointAt(mRayVisualizationSettings.maxDistance - length);
                    length = mRayVisualizationSettings.maxDistance;
                }
                currentLine.endAlpha = 0;

                mRayVisualizationLines.push_back(currentLine);
            }
            return;
        }
        return;
    }

    void GenerateLines(Vector3f origin, Colour colour) {
        int divisions = mRayVisualizationSettings.initialRays;
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
            line.colour = colour;
            TraceVisRay(ray, line, 0, 0, rnd);
        }
    }

    void GenerateVisualization() {
        mRayVisualizationLines.clear();
        for each (Object* object in mScene.GetLights()) {
            if (object->show) GenerateLines(object->GetPosition(), object->material.colour);
        }
    }

    void UpdateImGui() {
        // ImGui stuff (from example)
        ImGui_ImplSDLRenderer_NewFrame();
        ImGui_ImplSDL2_NewFrame();
        ImGui::NewFrame();

        // Enable docking
        ImGui::DockSpaceOverViewport();

        // Menu bar
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
                    if (ImGui::Checkbox("Dark Mode", &mDarkMode)) {
                        SetImGuiStyle(mDarkMode);
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
            ImGui::Text("Renderer: %.3f ms/frame (%.1f FPS)", mRendererFps * 1000, 1000/(mRendererFps * 1000));

            ImGui::End();
        }

        // Camera position
        {
            ImGui::Begin("Camera");
            mRedrawThisFrame |= ImGui::DragFloat3("Position", (float*)&mScene.camera.position, 0.01);

            ImGui::End();
        }

        // Object selection window
        {
            ImGui::Begin("Objects");
            if (ImGui::Button("Add New Object", { ImGui::GetWindowWidth(), 20 })) {
                ImGui::OpenPopup("Add Object");
            }

            // New object pop-up
            ImGui::SetNextWindowPos(ImGui::GetWindowPos(), ImGuiCond_Always, ImVec2(1, 0));
            if (ImGui::BeginPopup("Add Object")) {

                char nameBuffer[100];
                sprintf_s(nameBuffer, mNewObjectName.c_str(), 100);
                ImGui::InputText("Name", nameBuffer, 100);
                mNewObjectName = nameBuffer;

                const char* objectTypes[] = { "Sphere", "Cube", "3D Model", "Diverging Lens"};
                if (ImGui::BeginCombo("Material Type", objectTypes[mNewObjectType])) {
                    for (int i = 0; i < IM_ARRAYSIZE(objectTypes); i++) {
                        const bool selected = (mNewObjectType == i);
                        if (ImGui::Selectable(objectTypes[i], selected)) {
                            mNewObjectType = i;
                        }

                        if (selected)
                            ImGui::SetItemDefaultFocus();
                    }
                    ImGui::EndCombo();
                }

                bool valid = true;

                if (objectTypes[mNewObjectType] == "3D Model") {
                    char buffer[100];
                    sprintf_s(buffer, mNewObjectPath.c_str(), mNewObjectPath.length());
                    std::ifstream file(mNewObjectPath);
                    valid = file.good();
                    if (!valid) ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1, 0, 0, 1));
                    ImGui::InputText("File Path", buffer, 100);
                    if (!valid) ImGui::PopStyleColor();
                    mNewObjectPath = buffer;

                    if (ImGui::Button("Browse for file")) {
                        nfdchar_t* path = NULL;
                        nfdresult_t result = NFD_OpenDialog("obj", NULL, &path);
                        mNewObjectPath = AbsolutePathToRelative(path);
                    }
                }

                if (ImGui::Button("Ok") && valid) {
                    ImGui::CloseCurrentPopup();
                    Object* object;

                    if (objectTypes[mNewObjectType] == "Sphere") {
                        object = (Object*) new Sphere(Vector3f(), 0.5, Material());
                    }
                    else if (objectTypes[mNewObjectType] == "Cube") {
                        object = (Object*) new Mesh({ 0, 0, 0 }, "Cube.obj", Material());
                    }
                    else if (objectTypes[mNewObjectType] == "3D Model") {
                        object = (Object*) new Mesh({ 0, 0, 0 }, mNewObjectPath.c_str(), Material());
                    }
                    else if (objectTypes[mNewObjectType] == "Diverging Lens") {
                        object = (Object*) new DivergingLens({ 0, 0, 0 }, 0.1, 2, Material());
                    }

                    mRedrawThisFrame = true;
                    mScene.AddObject(mNewObjectName.c_str(), object);
                    mSelectedObject = mScene.GetObjectCount() - 1;
                    mNewObjectName = "Object " + std::to_string(mScene.GetObjectCount());
                    mNewObjectPath = "";
                    mNewObjectType = 0;
                }
                if (!valid && ImGui::IsItemHovered()) ImGui::SetMouseCursor(ImGuiMouseCursor_NotAllowed);

                ImGui::SameLine();
                if (ImGui::Button("Close")) ImGui::CloseCurrentPopup();

                ImGui::EndPopup();
            }

            // Add each objects to the list
            for (int n = 0; n < mScene.GetObjectCount(); n++) {
                ImGui::SetItemAllowOverlap();
                ImGui::SetCursorPos(ImVec2(8, n * 25 + 20 + 35));

                Object* obj = mScene.GetObjects()[n];

                // Create hidden id for the checkbox.
                char buf[32];

                // Hidden id for the checkbox
                sprintf(buf, "##Show %d", n + 1);

                // Checkbox to show and hide object.
                if (ImGui::Checkbox(buf, &obj->show)) {
                    mRedrawThisFrame = true;
                    if (!obj->show) mSelectedObject = -1;
                }

                ImGui::SetCursorPos(ImVec2(35, n * 25 + 20 + 3 + 35));

                // Create tag for the object (needs an id so that each label is unique)
                sprintf(buf, (std::string("##object_id_") + std::to_string(mScene.GetObjectID(obj))).c_str(), n + 1);

                // Set currently selected object.
                if (ImGui::Selectable(buf, mSelectedObject == n)) mSelectedObject = n;

                // Draw label
                ImGui::SameLine();
                ImGui::Text(obj->name.c_str());
            }

            ImGui::End();
        }

        // Current object properties
        {
            ImGui::Begin("Object Properties");

            // Make sure an object is selected
            if (mSelectedObject >= 0) {
                Object* obj = mScene.GetObjects()[mSelectedObject];

                char buffer[32];
                sprintf_s(buffer, obj->name.c_str(), obj->name.length());

                ImGui::InputText("Name", buffer, 32);

                Vector3f position = obj->GetPosition();
                Vector3f rotation = obj->GetRotation();
                Vector3f scale = obj->GetScale();

                if (ImGui::DragFloat3("Position", (float*)&position, 0.01)) {
                    mRedrawThisFrame = true;
                    obj->SetPosition(position);
                }

                if (obj->GetType() != ObjectType::Sphere) {
                    if (ImGui::DragFloat3("Rotation", (float*)&rotation, 1)) {
                        mRedrawThisFrame = true;
                        rotation.x = fmod(rotation.x, 360);
                        rotation.y = fmod(rotation.y, 360);
                        rotation.z = fmod(rotation.z, 360);
                        obj->SetRotation(rotation);
                    }
                }

                if (obj->GetType() == ObjectType::Sphere) {
                    if (ImGui::DragFloat("Radius", (float*)&scale.x, 0.01, 0.01, 100)) {
                        mRedrawThisFrame = true;
                        obj->SetScale(scale);
                    }
                }
                else if (obj->GetType() == ObjectType::DivergingLens) {
                    DivergingLens* lens = (DivergingLens*)obj;
                    float width = lens->GetWidth();
                    float curvature = lens->GetCurvature();

                    if (ImGui::DragFloat("Scale", (float*)&scale.x, 0.01, 0.01, 100)) {
                        mRedrawThisFrame = true;
                        obj->SetScale(scale);
                    }
                    if (ImGui::DragFloat("Width", (float*)&width, 0.01, 0.01, 1)) {
                        mRedrawThisFrame = true;
                        lens->SetWidth(width);
                    }
                    if (ImGui::DragFloat("Curvature", (float*)&curvature, 0.01, 0.01, 2)) {
                        mRedrawThisFrame = true;
                        lens->SetCurvature(curvature);
                    }
                }
                else {
                    ImGui::PushItemWidth(ImGui::GetWindowWidth() * 0.45);
                    if (ImGui::DragFloat3("Scale", (float*)&scale, 0.01, 0.01, 100)) {
                        mRedrawThisFrame = true;
                        if (obj->lockAspectRatio) {
                            Vector3f difference = scale - obj->GetScale();
                            for (int i = 0; i < 3; i++) {
                                if (difference[i] != 0) {
                                    float factor = scale[i] / obj->GetScale()[i];
                                    scale = obj->GetScale() * factor;
                                }
                            }
                        }
                        obj->SetScale(scale);
                    }
                    ImGui::PopItemWidth();
                    ImGui::SameLine();
                    ImGui::SetCursorPos(ImVec2(ImGui::GetWindowWidth() * 0.65 - 14, ImGui::GetCursorPosY()));
                    ImGui::PushItemWidth(ImGui::GetWindowWidth() * 0.65 + 10);
                    ImGui::Checkbox("Lock aspect ratio", &obj->lockAspectRatio);
                    ImGui::PopItemWidth();
                }


                obj->name = std::string(buffer);

                if (ImGui::Button("Delete")) {
                    mScene.RemoveObject(obj);
                    mSelectedObject--;
                    mRedrawThisFrame = true;
                }
            }
            else {
                ImGui::Text("Select an object to edit it's properties.");
            }
            ImGui::End();
        }

        // Object material properties
        {
            ImGui::Begin("Material Editor");

            // Make sure an object is selected
            if (mSelectedObject >= 0) {
                Object* obj = mScene.GetObjects()[mSelectedObject];
                Material* material = &obj->material;

                const char* materials[] = { "Lambertian", "Specular", "Glass" };
                // Pass in the preview value visible before opening the combo (it could be anything)
                const char* comboText = materials[(int)material->materialType]; 
                if (ImGui::BeginCombo("Material Type", comboText)) {
                    for (int i = 0; i < IM_ARRAYSIZE(materials); i++) {
                        bool selected = ((int)material->materialType == i);
                        if (ImGui::Selectable(materials[i], selected)) {
                            // Change material type
                            material->materialType = (MaterialType)i;
                            mRedrawThisFrame = true;
                        }

                        if (selected) ImGui::SetItemDefaultFocus();
                    }
                    ImGui::EndCombo();
                }

                mRedrawThisFrame |= ImGui::ColorEdit3("Colour", (float*)&material->colour);
                if (material->materialType != MaterialType::Glass) mRedrawThisFrame |= ImGui::DragFloat("Emitted", (float*)&material->emitted, 0.1, 0, 100);
                // TODO: albedo
                if (material->materialType == MaterialType::Glass) mRedrawThisFrame |= ImGui::SliderFloat("Index of Refraction", &material->refractiveIndex, 1, 2);
                if (material->materialType != MaterialType::Lambertian) mRedrawThisFrame |= ImGui::SliderFloat("Roughness", &material->roughness, 0, 1000);
            }
            else {
                ImGui::Text("Select an object to edit it's material.");
            }
            ImGui::End();
        }

        // Render settings
        {
            ImGui::Begin("Render Settings");
            if (ImGui::BeginTabBar("Render Settings Tab Bar")) {
                if (ImGui::BeginTabItem("Preview")) {
                    mRedrawThisFrame |= ImGui::SliderInt("Max Depth", &mPreviewRenderSettings.maxDepth, 1, 100);
                    mRedrawThisFrame |= ImGui::SliderInt("Samples Per Frame", &mPreviewRenderSettings.samples, 1, 100);
                    mRedrawThisFrame |= ImGui::ColorEdit3("Ambient Light Colour", (float*)&mPreviewRenderSettings.ambientLight);
                    mRedrawThisFrame |= ImGui::Checkbox("Checkerboard", &mPreviewRenderSettings.checkerboard);
                    mRedrawThisFrame |= ImGui::Checkbox("Explicit Light Sampling", &mPreviewRenderSettings.directLightSampling);
                    ImGui::EndTabItem();
                }
                ImGui::EndTabBar();
            }

            ImGui::End();
        }

        // Viewport
        {
            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
            ImGui::Begin("Scene");
            ImGui::SetCursorPos({ 0, 0 });

            ImGui::SetItemAllowOverlap();

            SDL_Point size;
            SDL_QueryTexture(mOutputTexture->GetRawTexture(), NULL, NULL, &size.x, &size.y);

            // Scale texture to fit in viewport
            float scale = (ImGui::GetWindowWidth()) / size.x;
            if (scale * size.y > ImGui::GetWindowHeight()) scale = (ImGui::GetWindowHeight()) / size.y;

            // Get position of top left corner of image
            ImVec2 textureTopLeftInWindow = ImGui::GetCursorScreenPos();

            // Add the texture
            ImGui::Image((void*)(intptr_t)mOutputTexture->GetRawTexture(), ImVec2(size.x * scale, size.y * scale));

            // The image is clicked
            if (ImGui::IsWindowHovered() && ImGui::IsMouseClicked(0) && ImGui::IsMouseHoveringRect(textureTopLeftInWindow, { textureTopLeftInWindow.x + size.x * scale, textureTopLeftInWindow.y + size.y * scale })) {
                // Work out which object is being clicked and select it
                ImVec2 pos = ImGui::GetMousePos();
                Vector2f relativePos = { (pos.x - textureTopLeftInWindow.x) / (scale * size.x), (pos.y - textureTopLeftInWindow.y) / (scale * size.y) };
                Vector3f viewportPos = mScene.camera.GetViewportPos(relativePos);
                Ray ray = Ray(mScene.camera.position, viewportPos);
                RayPayload payload;
                if (mScene.ClosestHit(ray, 0.001, FLT_MAX, payload)) {
                    mSelectedObject = mScene.GetObjectID(payload.object);
                }
                else {
                    // De-select
                    mSelectedObject = -1;
                }
            }

            ImGui::End();
            ImGui::PopStyleVar();
        }

        // Ray visualization
        {
            ImGui::Begin("Ray Visualization");
            ImGui::Checkbox("Show visualization", &mRayVisualizationSettings.enable);

            ImGui::Checkbox("Ignore sky", &mRayVisualizationSettings.ignoreSky);
            ImGui::SameLine();
            ImGui::Checkbox("Ignore ground", &mRayVisualizationSettings.ignoreGround);

            ImGui::Checkbox("Shorten infinite rays", &mRayVisualizationSettings.shortenRays);
            ImGui::SameLine();
            ImGui::SetCursorPos(ImVec2(160, ImGui::GetCursorPosY()));
            ImGui::PushItemWidth(ImGui::GetWindowWidth() * 0.65 - 160 + 10);
            ImGui::DragFloat("Shortened ray length", &mRayVisualizationSettings.shortenedRayLength, 0.01, 0.01, mRayVisualizationSettings.maxDistance);
            ImGui::PopItemWidth();

            ImGui::Checkbox("Update regularly", &mRayVisualizationSettings.updateRegularly);
            ImGui::SameLine();
            ImGui::SetCursorPos(ImVec2(160, ImGui::GetCursorPosY()));
            ImGui::PushItemWidth(ImGui::GetWindowWidth() * 0.65 - 160 + 10);
            ImGui::DragInt("Update Frames", &mRayVisualizationSettings.framesPerUpdate, 1, 1, 10);
            ImGui::PopItemWidth();

            ImGui::DragInt("Initial rays cast", &mRayVisualizationSettings.initialRays, 1, 0, 100);
            ImGui::DragInt("Max depth", &mRayVisualizationSettings.maxBounceDepth, 1, 0, 100);
            ImGui::DragFloat("Max distance", &mRayVisualizationSettings.maxDistance, 0.01, 0.01, 10);

            ImGui::Checkbox("Same colour", &mRayVisualizationSettings.sameColour);
            ImGui::SameLine();
            ImGui::SetCursorPos(ImVec2(160, ImGui::GetCursorPosY()));
            ImGui::PushItemWidth(ImGui::GetWindowWidth() * 0.65 - 160 + 10);
            ImGui::ColorEdit3("Colour", (float*)&mRayVisualizationSettings.lineColour);
            ImGui::PopItemWidth();

            if (ImGui::Button("Generate Rays")) GenerateVisualization();
            ImGui::End();
        }

        // Draw to the screen
        ImGui::Render();
        ImGui_ImplSDLRenderer_RenderDrawData(ImGui::GetDrawData());
    }

    void DrawRay(Texture* image, Line2D line, Line3D line3d) {
        // Use the Bresenham algorthithm to draw the ray onto the screen.

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
                if (d1 - d2 > 0) yCoord += yDirection;

                if (alpha < 0) alpha = 0;
                if (alpha > 1) alpha = 1;

                // Make sure the line is between the start and end points.
                if (xCoord >= 0 && xCoord < image->GetWidth() && yCoord >= 0 && yCoord < image->GetHeight()) {

                    // Get the position of the line through that pixel.
                    Vector3f toLine = mScene.camera.GetViewportPos({ (float)xCoord, (float)yCoord });
                    toLine.Normalize();
                    float t = (Vector2f((float)xCoord, (float)yCoord) - line.start).Magnitude() / (line.end - line.start).Magnitude();
                    Vector3f pos = line3d.start + t * (line3d.end - line3d.start);

                    // Only draw if the line is in front of every other object.
                    if ((pos - mScene.camera.position).Magnitude() < mDepthBufferOld[xCoord][yCoord] || mDepthBufferOld[xCoord][yCoord] < 0) {
                        // Alpha blend between the line colour and the image colour.
                        colour = line.colour * alpha + image->GetColourAt({ (float)xCoord, (float)yCoord }) / 255 * (1 - alpha);
                        // Multiply by the image colour if the object is glass.
                        if (mDepthBufferOld[xCoord][yCoord] < 0 && (pos - mScene.camera.position).Magnitude() > abs(mDepthBufferOld[xCoord][yCoord]))
                            colour = colour * image->GetColourAt({ (float)xCoord, (float)yCoord }) / 255;
                        colour *= 255;
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
                    Vector3f toLine = mScene.camera.GetViewportPos({ (float)xCoord, (float)yCoord });
                    toLine.Normalize();

                    float t = (Vector2f((float)xCoord, (float)yCoord) - line.start).Magnitude() / (line.end - line.start).Magnitude();
                    Vector3f pos = line3d.start + t * (line3d.end - line3d.start);

                    if ((pos - mScene.camera.position).Magnitude() < mDepthBufferOld[xCoord][yCoord] || mDepthBufferOld[xCoord][yCoord] < 0) {
                        colour = line.colour * alpha + image->GetColourAt({ (float)xCoord, (float)yCoord }) / 255 * (1 - alpha);
                        if (mDepthBufferOld[xCoord][yCoord] < 0 && (pos - mScene.camera.position).Magnitude() > abs(mDepthBufferOld[xCoord][yCoord]))
                            colour = colour * image->GetColourAt({ (float)xCoord, (float)yCoord }) / 255;
                        colour *= 255;
                        image->SetColourAt({ (float)xCoord, (float)yCoord }, colour);
                    }
                }
                alpha += alphaDiff;
            }
        }
        image->Unlock();
    }

    void CalculateObjectVisability() {
        // Check which pixels contain the selected object.
        for (int y = 0; y < mPreviewRenderSettings.resolution.y; y++) {
            for (int x = 0; x < mPreviewRenderSettings.resolution.x; x++) {
                // If an object is selected.
                if (mSelectedObject >= 0) {
                    Vector2f screenPos = { x / mPreviewRenderSettings.resolution.x, y / mPreviewRenderSettings.resolution.y };
                    Vector3f viewportPos = mScene.camera.GetViewportPos(screenPos);
                    Ray ray = Ray(mScene.camera.position, viewportPos);
                    Object* selected = mScene.GetObjects()[mSelectedObject];
                    RayPayload payload;
                    mIsSelectedObjectVisible[x][y] = selected->Intersect(ray, 0, FLT_MAX, payload);
                }
                else {
                    mIsSelectedObjectVisible[x][y] = false;
                }
            }
        }
    }

    void ConvertRenderedImageToTexture(Colour** image, Texture* texture) {
        texture->Lock();
        for (int y = 0; y < mPreviewRenderSettings.resolution.y; y++) {
            for (int x = 0; x < mPreviewRenderSettings.resolution.x; x++) {
                Colour colour = Vector3f();

                // If checkerboard rendering is enabled, fill in the empty squares.
                if (mPreviewRenderSettings.checkerboard && !((x % 2 == 0 && y % 2 == 0) || (x % 2 != 0 && y % 2 != 0))) {
                    int samples = 0;
                    if (x - 1 >= 0) {
                        colour += image[x - 1][y];
                        samples++;
                    }
                    if (x + 1 < mPreviewRenderSettings.resolution.x) {
                        colour += image[x + 1][y];
                        samples++;
                    }
                    if (y - 1 >= 0) {
                        colour += image[x][y - 1];
                        samples++;
                    }
                    if (y + 1 < mPreviewRenderSettings.resolution.y) {
                        colour += image[x][y + 1];
                        samples++;
                    }

                    colour /= samples;
                }
                else {
                    colour = image[x][y];
                }

                // Set texture pixel.
                texture->SetColourAt({ (float)x, (float)y }, colour * 255);
            }
        }
        texture->Unlock();
    }

    void DrawSelectedObjectOutline(Texture* texture) {
        texture->Lock();
        for (int y = 0; y < mPreviewRenderSettings.resolution.y; y++) {
            for (int x = 0; x < mPreviewRenderSettings.resolution.x; x++) {
                // If the pixel contains the selected object, fill all surrounding pixels that don't contain the selected object.
                if (mIsSelectedObjectVisible[x][y]) {
                    for (int xOffset = x - 1; xOffset <= x + 1; xOffset++) {
                        for (int yOffset = y - 1; yOffset <= y + 1; yOffset++) {
                            if (xOffset >= 0 && xOffset < mPreviewRenderSettings.resolution.x && yOffset >= 0 && yOffset < mPreviewRenderSettings.resolution.y) {
                                if (!mIsSelectedObjectVisible[xOffset][yOffset]) {
                                    texture->SetColourAt({ (float)xOffset, (float)yOffset }, { 0, 0, 0 });
                                }
                            }
                        }
                    }
                }
            }
        }
        texture->Unlock();
    }

    void GetUserInput() {
        // Process input for ImGui (from ImGui example).
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

        // Keyboard shortcuts.
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
    }

    void DrawRayVisualization() {
        for each (Line3D line in mRayVisualizationLines) {
            // Make sure line is visible.
            if ((line.start - mScene.camera.position).Dot(Vector3f(0, 0, -1)) <= 0 || (line.end - mScene.camera.position).Dot(Vector3f(0, 0, -1)) <= 0) continue;

            // Calculate 2d line position.
            Line2D line2d = { mScene.camera.GetScreenPos(line.start) * mPreviewRenderSettings.resolution, mScene.camera.GetScreenPos(line.end) * mPreviewRenderSettings.resolution, line.colour, line.startAlpha, line.endAlpha };
            if (mRayVisualizationSettings.sameColour) line2d.colour = mRayVisualizationSettings.lineColour;

            // Line must start and end exactly on the pixel.
            line2d.start.x = (int)line2d.start.x;
            line2d.start.y = (int)line2d.start.y;
            line2d.end.x = (int)line2d.end.x;
            line2d.end.y = (int)line2d.end.y;

            DrawRay(mOutputTexture, line2d, line);
        }
    }

    void Update(float deltaTime) {
        // Update the window title to show the name of the scene
        std::string title = "Ray Tracing Optics Simulator: ";
        if (mScene.GetName() != "") title += mScene.GetName();
        else title += "unnamed";
        // Add a * if the file has unsaved changes.
        if (mScene.IsModified()) title += "*";
        GetWindow()->SetTitle(title.c_str());

        mFrame++;

        // Update the visualization.
        if (mFrame % mRayVisualizationSettings.framesPerUpdate == 0 && mRayVisualizationSettings.updateRegularly) GenerateVisualization();

        // Set the scene as modified if changes were made.
        if (mRedrawThisFrame && !mLoadedSceneThisFrame) mScene.SetModified();

        // Start rendering again from scratch if settings are changed.
        if (mRedrawThisFrame) mFrame = 1;

        // Swap round the depth buffers, so that it doesn't make the ray visualization lines flicker.
        if (mRedrawThisFrame) {
            float** temp = mDepthBuffer;
            mDepthBuffer = mDepthBufferOld;
            mDepthBufferOld = temp;
        }

        // Update object transforms
        mScene.UpdateObjects();

        // Render the image and calculate the time takes.
        std::chrono::system_clock::time_point previousTime = std::chrono::system_clock::now();
        mRenderedImage = GetRenderer()->RenderScene(mScene, mRenderedImage, mDepthBuffer, mPreviewRenderSettings, mFrame);
        std::chrono::system_clock::time_point currentTime = std::chrono::system_clock::now();
        std::chrono::duration<float> elapsed = std::chrono::duration_cast<std::chrono::duration<float>>(currentTime - previousTime);
        mRendererFps = elapsed.count();

        // Swap depth buffers back round.
        if (mRedrawThisFrame || mLoadedSceneThisFrame) {
            float** temp = mDepthBuffer;
            mDepthBuffer = mDepthBufferOld;
            mDepthBufferOld = temp;
        }

        mRedrawThisFrame = false;
        mLoadedSceneThisFrame = false;

        CalculateObjectVisability();

        ConvertRenderedImageToTexture(mRenderedImage, mOutputTexture);
        if (mRayVisualizationSettings.enable) DrawRayVisualization();
        DrawSelectedObjectOutline(mOutputTexture);

        GetUserInput();

        // Draw the interface.
        GetRenderer()->Clear();
        UpdateImGui();
    }

private:
    Scene      mScene;

    bool       mDarkMode;

    int        mInternalWidth;
    int        mInternalHeight;
    float      mAspectRatio;

    int        mFrame = 0;
    float      mRendererFps;

    bool       mRedrawThisFrame;
    bool       mLoadedSceneThisFrame;

    Colour**   mRenderedImage;
    Texture*   mOutputTexture;

    bool**     mIsSelectedObjectVisible;
    float**    mDepthBuffer;
    float**    mDepthBufferOld;

    RenderSettings mPreviewRenderSettings;

    std::string   mNewObjectName; // Data for creating a new object.
    int           mNewObjectType;
    std::string   mNewObjectPath;
    int           mSelectedObject;

    RayVisualizationSettings  mRayVisualizationSettings;
    std::vector<Line3D>       mRayVisualizationLines;
};

int main(int argc, char* argv[]) {
    Application* app = new FinalApp(argv[1]);
    app->Run();
    return 0;
}