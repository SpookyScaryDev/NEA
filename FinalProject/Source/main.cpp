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

#include <windows.h>
#include <shellapi.h>

#include <Application/Application.h>
#include <Maths/Ray.h>
#include <Maths/Vector3f.h>
#include <Renderer/Texture.h>
#include <Renderer/Camera.h>
#include <Renderer/Sphere.h>
#include <Renderer/DivergingLens.h>
#include <Renderer/ConvergingLens.h>
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

    json ToJSON() {
        json data = json();

        data["enable"]             = enable;
        data["updateRegularly"]    = updateRegularly;
        data["framesPerUpdate"]    = framesPerUpdate;
        data["maxBounceDepth"]     = maxBounceDepth;
        data["initialRays"]        = initialRays;
        data["shortenRays"]        = shortenRays;
        data["shortenedRayLength"] = shortenedRayLength;
        data["ignoreSky"]          = ignoreSky;
        data["ignoreGround"]       = ignoreGround;
        data["maxDistance"]        = maxDistance;
        data["sameColour"]         = sameColour;
        data["lineColour"]         = lineColour.ToJSON();;

        return data;
    }

    void LoadFromJSON(json data) {
        enable                   = data["enable"];
        updateRegularly          = data["updateRegularly"];
        framesPerUpdate          = data["framesPerUpdate"];
        maxBounceDepth           = data["maxBounceDepth"];
        initialRays              = data["initialRays"];
        shortenRays              = data["shortenRays"];
        shortenedRayLength       = data["shortenedRayLength"];
        ignoreSky                = data["ignoreSky"];
        ignoreGround             = data["ignoreGround"];
        maxDistance              = data["maxDistance"];
        sameColour               = data["sameColour"];
        lineColour               = Vector3f::LoadFromJSON(data["lineColour"]);
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
        mPreviewRenderSettings.numThreads = 10;

        mOutputRenderSettings = RenderSettings();
        mOutputRenderSettings.resolution = { (float)mInternalWidth * 10, (float)mInternalHeight * 10};
        mOutputRenderSettings.maxDepth = 1000;
        mOutputRenderSettings.samples = 1000;
        mOutputRenderSettings.ambientLight = Vector3f(1, 1, 1);
        mOutputRenderSettings.checkerboard = true;
        mOutputRenderSettings.directLightSampling = false;
        mOutputRenderSettings.numThreads = 10;

        mRedrawThisFrame = false;
        mLoadedSceneThisFrame = true;

        mOutputImageDirectory = std::filesystem::current_path().string();
        mOutputImageName = "output";
        mOpenDirectoryOnComplete = false;
        mOpenFileOnComplete = true;

        // The image which will be rendered on the GUI.
        mOutputTexture = new Texture(mInternalWidth, mInternalHeight);
        SDL_SetTextureScaleMode(mOutputTexture->GetRawTexture(), SDL_ScaleModeBest); // TODO: Put in Texture class

        mDefaultCamera = Camera(mAspectRatio, 2, Vector3f());

        if (!file) New();
        else mScene = mScene = Scene::LoadFromFile(file);

        LoadSettings();

        mSelectedObject = -1;

        mNewObjectType = 0;
        mNewObjectPath = "";
        mNewObjectName = "Object " + std::to_string(mScene.GetObjectCount());

        mLookAtSelected = true;

        mMouseInViewport = false;
        mMouseDrag = false;

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
        mMonoFont = io.Fonts->AddFontDefault();

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
            style.Colors[ImGuiCol_TabActive] = ImVec4(0.1450980454683304f, 0.1450980454683304f, 0.1490196138620377f, 1.0f);
            style.Colors[ImGuiCol_TabHovered] = ImVec4(0.1137254908680916f, 0.5921568870544434f, 0.9254902005195618f, 1.0f);
            style.Colors[ImGuiCol_Tab] = ImVec4(0.1725490242242813f, 0.1725490242242813f, 0.1803921610116959f, 1.0f);
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

    void SaveSettings() {
        SaveSettingsAs(mScene.GetPath());
    }

    void SaveSettingsAs(std::string path) {
        path.replace(path.find(".scene"), sizeof(".scene") - 1, ".editor");

        std::ofstream file(path);
        if (file.good()) {
            json data;
            data["selectedObject"] = mSelectedObject;
            data["rayVisualizationSettings"] = mRayVisualizationSettings.ToJSON();
            data["previewRenderSettings"] = mPreviewRenderSettings.ToJSON();
            data["outputRenderSettings"] = mOutputRenderSettings.ToJSON();
            file << std::setw(4) << data << std::endl;
        }
        else {
            Error("SaveSettings: failed to save editor settings to " + path);
        }
        file.close();
    }

    void LoadSettings() {
        std::string path = mScene.GetPath();
        path.replace(path.find(".scene"), sizeof(".scene") - 1, ".editor");

        std::ifstream file(path);
        if (file.good()) {
            json data = json::parse(file);
            if (data.contains("selectedObject")) mSelectedObject = data["selectedObject"];
            mRayVisualizationSettings.LoadFromJSON(data["rayVisualizationSettings"]);
            mPreviewRenderSettings.LoadFromJSON(data["previewRenderSettings"]);
            mOutputRenderSettings.LoadFromJSON(data["outputRenderSettings"]);
        }
        else {
            Error("LoadSettings: No editor settings file found at " + path);
        }
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

    bool GetDirectory(std::string defaultPath, std::string& filePath) {
        nfdchar_t* path = NULL;
        nfdresult_t result = NFD_PickFolder(defaultPath.c_str(), &path);
        if (result == nfdresult_t::NFD_ERROR) {
            Error(std::string("NFD: Failed open directory dialog! ") + NFD_GetError());
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
        mLoadedSceneThisFrame = true;
        mScene = mScene.LoadFromFile("Default/default.scene");

        mSelectedObject = -1;

        mNewObjectType = 0;
        mNewObjectPath = "";
        mNewObjectName = "Object " + std::to_string(mScene.GetObjectCount());

        LoadSettings();

        GenerateVisualization();
    }

    void Save() {
        if (mScene.GetName() != "default") {
            mScene.Save();
            SaveSettings();
        }
        else SaveSceneAs();
    }

    void SaveSceneAs() {
        std::string path;
        if (SaveAs("scene", path)) {
            mScene.SaveToFile((path + std::string(".scene")).c_str());
            SaveSettingsAs(path + std::string(".scene"));
        }
    }

    void OpenScene() {
        std::string path;
        if (OpenFile("scene", path)) {
            path = AbsolutePathToRelative(path);
            mScene = mScene.LoadFromFile(path.c_str());
            mRedrawThisFrame = true;
            mLoadedSceneThisFrame = true;
            LoadSettings();
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
                Vector3f direction = Vector3f(sin(i) * cos(j), sin(j) * sin(i), cos(i));
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

    void RunTests() {
        // 2D vector tests
        ImGui::Text("2D Vector tests");

        Vector2f v2f1 = { 2, 4.3 };
        Vector2f v2f2 = { 0, -3 };

        ImGui::Text(" Adding 2D vectors:                    ");
        ImGui::SameLine();
        ImGui::Text((v2f1.ToString() + " + " + v2f2.ToString() + " = " + (v2f1 + v2f2).ToString()).c_str());

        ImGui::Text(" Subtracting 2D vectors:               ");
        ImGui::SameLine();
        ImGui::Text((v2f1.ToString() + " - " + v2f2.ToString() + " = " + (v2f1 - v2f2).ToString()).c_str());

        ImGui::Text(" Multiplying 2D vectors by a scalar:   ");
        ImGui::SameLine();
        ImGui::Text(("2 * " + v2f2.ToString() + " = " + (2 * v2f2).ToString()).c_str());

        ImGui::Text(" Dividing 2D vectors by a scalar:      ");
        ImGui::SameLine();
        ImGui::Text((v2f2.ToString() + " / 2 = " + (v2f2 / 2).ToString()).c_str());

        ImGui::Text(" Dotting 2D vectors:                   ");
        ImGui::SameLine();
        ImGui::Text((v2f1.ToString() + " . " + v2f2.ToString() + " = " + std::to_string(v2f1.Dot(v2f2))).c_str());

        ImGui::Text(" 2D vector magnitude:                  ");
        ImGui::SameLine();
        ImGui::Text(("|" + v2f1.ToString() + "|" + " = " + std::to_string(v2f1.Magnitude())).c_str());

        Vector2f v2f1Norm = v2f1;
        v2f1Norm.Normalize();
        ImGui::Text(" 2D vector normalization:              ");
        ImGui::SameLine();
        ImGui::Text(("The unit vector of " + v2f1.ToString() + " is " + v2f1Norm.ToString()).c_str());

        ImGui::NewLine();

        // 3D vector tests
        ImGui::Text("3D Vector tests");

        Vector3f v3f1 = { 5.01, 10.2, -0.01 };
        Vector3f v3f2 = { 0, -24, -3.4 };

        ImGui::Text(" Adding 3D vectors:                    ");
        ImGui::SameLine();
        ImGui::Text((v3f1.ToString() + " + " + v3f2.ToString() + " = " + (v3f1 + v3f2).ToString()).c_str());

        ImGui::Text(" Subtracting 3D vectors:               ");
        ImGui::SameLine();
        ImGui::Text((v3f1.ToString() + " - " + v3f2.ToString() + " = " + (v3f1 - v3f2).ToString()).c_str());

        ImGui::Text(" Multiplying 3D vectors by a scalar:   ");
        ImGui::SameLine();
        ImGui::Text(("2 * " + v3f2.ToString() + " = " + (2 * v3f2).ToString()).c_str());

        ImGui::Text(" Dividing 3D vectors by a scalar:      ");
        ImGui::SameLine();
        ImGui::Text((v3f2.ToString() + " / 2 = " + (v3f2 / 2).ToString()).c_str());

        ImGui::Text(" Dotting 3D vectors:                   ");
        ImGui::SameLine();
        ImGui::Text((v3f1.ToString() + " . " + v3f2.ToString() + " = " + std::to_string(v3f1.Dot(v3f2))).c_str());

        ImGui::Text(" Crossing 3D vectors:                  ");
        ImGui::SameLine();
        ImGui::Text((v3f1.ToString() + " x " + v3f2.ToString() + " = " + (v3f1.Cross(v3f2)).ToString()).c_str());

        ImGui::Text(" 3D vector magnitude:                  ");
        ImGui::SameLine();
        ImGui::Text(("|" + v3f1.ToString() + "|" + " = " + std::to_string(v3f1.Magnitude())).c_str());

        Vector3f v3f1Norm = v3f1;
        v3f1Norm.Normalize();
        ImGui::Text(" 3D vector normalization:              ");
        ImGui::SameLine();
        ImGui::Text(("The unit vector of " + v3f1.ToString() + " is " + v3f1Norm.ToString()).c_str());

        ImGui::NewLine();

        // Matrix tests
        ImGui::Text("4 by 4 matrix tests");

        Matrix4x4f translate = Matrix4x4f::Translate({ 3, 1, -5.2 });
        Matrix4x4f rotate = Matrix4x4f::Rotate({ 30, 45, 107.3 });
        Matrix4x4f scale = Matrix4x4f::Scale({ 2, 1.4, -4.2 });
        Matrix4x4f identity = Matrix4x4f::Identity();
        Matrix4x4f zero = Matrix4x4f::Zero();

        ImGui::Text(" Zero matrix:                          ");
        ImGui::SameLine();
        ImGui::Text((zero.ToString()).c_str());

        ImGui::Text(" Identity matrix:                      ");
        ImGui::SameLine();
        ImGui::Text((identity.ToString()).c_str());

        ImGui::Text(" Translation matrix from (3, 1, -5.2): ");
        ImGui::SameLine();
        ImGui::Text((translate.ToString()).c_str());

        ImGui::Text(" Rotation matrix from (30, 45, 107.3): ");
        ImGui::SameLine();
        ImGui::Text((rotate.ToString()).c_str());

        ImGui::Text(" Scale matrix from (2, 1.4, -4.2):     ");
        ImGui::SameLine();
        ImGui::Text((scale.ToString()).c_str());
    }

    void UIMenuBar() {
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

            if (ImGui::BeginMenu("Testing")) {
                if (ImGui::Button("Begin Tests")) ImGui::OpenPopup("Testing");
                ImGui::SetNextWindowPos(ImVec2(ImGui::GetIO().DisplaySize.x / 2, ImGui::GetIO().DisplaySize.y / 2), ImGuiCond_Always, ImVec2(0.5, 0.5));
                if (ImGui::BeginPopup("Testing")) {
                    ImGui::PushFont(mMonoFont);
                    RunTests();
                    ImGui::PopFont();
                    ImGui::EndPopup();
                }
                ImGui::EndMenu();
            }

            ImGui::EndMainMenuBar();
        }
    }

    void UIFpsOverlay() {
        ImGui::SetNextWindowBgAlpha(0.9f);
        ImGui::Begin("Overlay", (bool*)1, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoDocking);
        ImGui::Text("%.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
        ImGui::Text("Renderer: %.3f ms/frame (%.1f FPS)", mRendererFps * 1000, 1000 / (mRendererFps * 1000));

        ImGui::End();
    }

    void UICamera() {
        ImGui::Begin("Camera");
        Vector3f position = mScene.camera.GetPosition();

        if (ImGui::DragFloat3("Position", (float*)&position, 0.01)) {
            mRedrawThisFrame = true;
            mScene.camera.SetPosition(position);
        }

        mRedrawThisFrame |= ImGui::Checkbox("Look at selected object", &mLookAtSelected);
        if (ImGui::Button("Reset view")) {
            mRedrawThisFrame = true;
            mScene.camera.LookAt(mScene.camera.GetPosition() + Vector3f(0, 0, -1));
        }

        ImGui::End();
    }

    void UIObjectSelect() {
        ImGui::Begin("Objects");
        if (ImGui::Button("Add New Object", { ImGui::GetWindowWidth(), 20 })) {
            ImGui::OpenPopup("Add Object");
        }

        // New object pop-up
        ImGui::SetNextWindowPos(ImGui::GetWindowPos(), ImGuiCond_Always, ImVec2(1, 0));
        if (ImGui::BeginPopup("Add Object")) {

            char nameBuffer[30];
            sprintf_s(nameBuffer, mNewObjectName.c_str(), 30);
            ImGui::InputText("Name", nameBuffer, 30);
            mNewObjectName = nameBuffer;

            const char* objectTypes[] = { "Sphere", "Cube", "Prism", "Diverging Lens", "Converging Lens", "3D Model" };
            if (ImGui::BeginCombo("Object Type", objectTypes[mNewObjectType])) {
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
                    if (result == nfdresult_t::NFD_OKAY) mNewObjectPath = AbsolutePathToRelative(path);
                }
            }

            if (ImGui::Button("Ok") && valid) {
                ImGui::CloseCurrentPopup();
                Object* object;

                if (objectTypes[mNewObjectType] == "Sphere") {
                    object = (Object*) new Sphere(Vector3f(), 1, Material());
                }
                else if (objectTypes[mNewObjectType] == "Cube") {
                    object = (Object*) new Mesh({ 0, 0, 0 }, "Models/Cube.obj", Material());
                }
                else if (objectTypes[mNewObjectType] == "Prism") {
                    object = (Object*) new Mesh({ 0, 0, 0 }, "Models/Prism.obj", Material());
                }
                else if (objectTypes[mNewObjectType] == "3D Model") {
                    object = (Object*) new Mesh({ 0, 0, 0 }, mNewObjectPath.c_str(), Material());
                }
                else if (objectTypes[mNewObjectType] == "Diverging Lens") {
                    object = (Object*) new DivergingLens({ 0, 0, 0 }, 0.11, 1.48, Material());
                }
                else if (objectTypes[mNewObjectType] == "Converging Lens") {
                    object = (Object*) new ConvergingLens({ 0, 0, 0 }, 0.5, Material());
                }

                object->material = Material::LoadFromPreset(MaterialPreset::Paper);

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
            if (ImGui::Selectable(buf, mSelectedObject == n)) {
                mSelectedObject = n;
                if (mLookAtSelected) mRedrawThisFrame = true;
            }

            // Draw label
            ImGui::SameLine();
            ImGui::Text(obj->name.c_str());
        }

        ImGui::End();
    }

    void UIObjectProperties() {
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

            if (ImGui::DragFloat3("Position (x, y, z)", (float*)&position, 0.01)) {
                mRedrawThisFrame = true;
                obj->SetPosition(position);
            }

            if (obj->GetType() != ObjectType::Sphere) {
                if (ImGui::DragFloat3("Rotation (x, y, z)", (float*)&rotation, 1)) {
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
                    if (scale.x <= 0) scale.x = 0.01;
                    obj->SetScale(scale);
                }
                if (ImGui::DragFloat("Thickness", (float*)&width, 0.01, 0.01, 1)) {
                    mRedrawThisFrame = true;
                    if (width <= 0) width = 0.01;
                    if (width > 1) width = 1;
                    lens->SetWidth(width);
                }
                if (ImGui::DragFloat("Curvature", (float*)&curvature, 0.01, 0.01, 2)) {
                    mRedrawThisFrame = true;
                    if (curvature <= 0) curvature = 0.01;
                    if (curvature > 2) curvature = 2;
                    lens->SetCurvature(curvature);
                }
            }
            else if (obj->GetType() == ObjectType::ConvergingLens) {
                ConvergingLens* lens = (ConvergingLens*)obj;
                float width = lens->GetWidth();

                if (ImGui::DragFloat("Scale", (float*)&scale.x, 0.01, 0.01, 100)) {
                    mRedrawThisFrame = true;
                    if (scale.x <= 0) scale.x = 0.01;
                    obj->SetScale(scale);
                }
                if (ImGui::DragFloat("Thickness", (float*)&width, 0.01, 0.01, 1)) {
                    mRedrawThisFrame = true;
                    if (width <= 0) width = 0.01;
                    if (width > 1) width = 1;
                    lens->SetWidth(width);
                }
            }
            else {
                ImGui::PushItemWidth(ImGui::GetWindowWidth() * 0.45);
                if (ImGui::DragFloat3("Scale", (float*)&scale, 0.01, 0.01, 100)) {

                    if (scale.x <= 0) scale.x = 0.01;
                    if (scale.y <= 0) scale.y = 0.01;
                    if (scale.z <= 0) scale.z = 0.01;

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
                ImGui::OpenPopup("Delete");
            }

            if (ImGui::BeginPopup("Delete")) {
                ImGui::Text("Are you sure?");
                if (ImGui::Button("Yes")) {
                    mScene.RemoveObject(obj);
                    mSelectedObject--;
                    mRedrawThisFrame = true;
                }
                ImGui::SameLine();
                if (ImGui::Button("Cancel")) ImGui::CloseCurrentPopup();
                ImGui::EndPopup();
            }

        }
        else {
            ImGui::Text("Select an object to edit it's properties.");
        }
        ImGui::End();
    }

    void UIMaterialProperties() {
        ImGui::Begin("Material Editor");

        // Make sure an object is selected
        if (mSelectedObject >= 0) {
            Object* obj = mScene.GetObjects()[mSelectedObject];
            Material* material = &obj->material;

            const char* presets[] = { "Paper", "Plastic", "Metal", "Glass", "FrostedGlass", "Light" };
            std::string currentPreset = "";
            for (int i = 0; i < IM_ARRAYSIZE(presets); i++) {
                if (*material == Material::LoadFromPreset((MaterialPreset)i)) {
                    currentPreset = presets[i];
                }
            }
            if (currentPreset == "") currentPreset = "Custom";
            if (ImGui::BeginCombo("Preset", currentPreset.c_str())) {
                for (int i = 0; i < IM_ARRAYSIZE(presets); i++) {
                    bool selected = ((int)material->materialType == i);
                    if (ImGui::Selectable(presets[i], selected)) {
                        // Change material type
                        *material = Material::LoadFromPreset((MaterialPreset)i);
                        mRedrawThisFrame = true;
                    }
                    if (selected) ImGui::SetItemDefaultFocus();
                }
                ImGui::EndCombo();
            }

            const char* materials[] = { "Lambertian", "Specular", "Glass" };
            // Pass in the preview value visible before opening the combo (it could be anything)
            const char* comboText = materials[(int)material->materialType];
            if (ImGui::BeginCombo("Material Type", comboText)) {
                for (int i = 0; i < IM_ARRAYSIZE(materials); i++) {
                    bool selected = ((int)material->materialType == i);
                    if (ImGui::Selectable(materials[i], selected)) {
                        // Change material type
                        if ((MaterialType)i != MaterialType::Lambertian) material->emitted = 0;
                        material->materialType = (MaterialType)i;
                        mRedrawThisFrame = true;
                    }

                    if (selected) ImGui::SetItemDefaultFocus();
                }
                ImGui::EndCombo();
            }

            mRedrawThisFrame |= ImGui::ColorEdit3("Colour", (float*)&material->colour);
            if (material->materialType == MaterialType::Lambertian) {
                mRedrawThisFrame |= ImGui::DragFloat("Emitted", (float*)&material->emitted, 0.01, 0, 1000);
                if (material->emitted <= 0) material->emitted = 0.01;
            }
            // TODO: albedo
            if (material->materialType == MaterialType::Glass) {
                mRedrawThisFrame |= ImGui::SliderFloat("Index of Refraction", &material->refractiveIndex, 1, 2);
                if (material->refractiveIndex < 1) material->refractiveIndex = 1;
                if (material->refractiveIndex > 2) material->refractiveIndex = 2;
            }
            if (material->materialType != MaterialType::Lambertian) {
                mRedrawThisFrame |= ImGui::DragFloat("Roughness", &material->roughness, 1, 0, 990);
                if (material->roughness < 1) material->roughness = 1;
                if (material->roughness < 990) material->roughness = 990;
            }
        }
        else {
            ImGui::Text("Select an object to edit it's material.");
        }
        ImGui::End();
    }

    void UIRenderSettings() {
        ImGui::Begin("Render Settings");
        if (ImGui::BeginTabBar("Render Settings Tab Bar")) {
            if (ImGui::BeginTabItem("Preview")) {
                const char* modes[] = { "Ray Tracer", "Depth Buffer", "Normals" };
                // Pass in the preview value visible before opening the combo (it could be anything)
                const char* comboText = modes[(int)mPreviewRenderSettings.mode];
                if (ImGui::BeginCombo("Rendering Mode", comboText)) {
                    for (int i = 0; i < IM_ARRAYSIZE(modes); i++) {
                        bool selected = ((int)mPreviewRenderSettings.mode == i);
                        if (ImGui::Selectable(modes[i], selected)) {
                            // Change material type
                            mPreviewRenderSettings.mode = (RenderMode)i;
                            mRedrawThisFrame = true;
                        }

                        if (selected) ImGui::SetItemDefaultFocus();
                    }
                    ImGui::EndCombo();
                }
                mRedrawThisFrame |= ImGui::Checkbox("Fast Mode", &mPreviewRenderSettings.fastMode);

                ImGui::NewLine();

                mRedrawThisFrame |= ImGui::DragInt("Max Bounce Depth", &mPreviewRenderSettings.maxDepth, 1, 1, 1000);
                if (mPreviewRenderSettings.maxDepth < 1) mPreviewRenderSettings.maxDepth = 1;
                if (mPreviewRenderSettings.maxDepth > 1000) mPreviewRenderSettings.maxDepth = 1000;

                mRedrawThisFrame |= ImGui::DragInt("Samples Per Frame", &mPreviewRenderSettings.samples, 0.5, 1, 10);
                if (mPreviewRenderSettings.samples < 1) mPreviewRenderSettings.samples = 1;
                if (mPreviewRenderSettings.samples > 10) mPreviewRenderSettings.samples = 10;

                mRedrawThisFrame |= ImGui::DragInt("Threads", &mPreviewRenderSettings.numThreads, 1, 1, 30);
                if (mPreviewRenderSettings.numThreads < 1) mPreviewRenderSettings.numThreads = 1;
                if (mPreviewRenderSettings.numThreads > 30) mPreviewRenderSettings.numThreads = 30;

                if (ImGui::ColorEdit3("Ambient Light Colour", (float*)&mPreviewRenderSettings.ambientLight)) {
                    mRedrawThisFrame = true;
                    mOutputRenderSettings.ambientLight = mPreviewRenderSettings.ambientLight;
                }

                mRedrawThisFrame |= ImGui::Checkbox("Checkerboard", &mPreviewRenderSettings.checkerboard);
                mRedrawThisFrame |= ImGui::Checkbox("Explicit Light Sampling", &mPreviewRenderSettings.directLightSampling);
                ImGui::EndTabItem();
            }
            if (ImGui::BeginTabItem("Render")) {
                char name[32];
                sprintf_s(name, mOutputImageName.c_str(), mOutputImageName.length());
                std::ifstream file(mOutputImageDirectory + "\\" + mOutputImageName + ".bmp");
                bool validName = !file.good();
                if (!validName) ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1, 0, 0, 1));
                ImGui::InputText("Name", name, 32);
                if (!validName) ImGui::PopStyleColor();
                mOutputImageName = name;

                char dir[100];
                sprintf_s(dir, mOutputImageDirectory.c_str(), mOutputImageDirectory.length());
                bool validDir = std::filesystem::exists(mOutputImageDirectory);
                if (!validDir) ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1, 0, 0, 1));
                ImGui::InputText("File Path", dir, 100);
                if (!validDir) ImGui::PopStyleColor();
                mOutputImageDirectory = dir;

                if (ImGui::Button("Browse for directory")) {
                    GetDirectory(mOutputImageDirectory, mOutputImageDirectory);
                }

                ImGui::Checkbox("Open directory when done", &mOpenDirectoryOnComplete);
                ImGui::Checkbox("Open file when done", &mOpenFileOnComplete);

                if (ImGui::Button("Render Image")) {
                    if (validName && validDir) RenderToImage();
                }

                if ((!validName || !validDir) && ImGui::IsItemHovered()) ImGui::SetMouseCursor(ImGuiMouseCursor_NotAllowed);

                ImGui::NewLine();

                const char* modes[] = { "Ray Tracer", "Depth Buffer", "Normals" };
                // Pass in the preview value visible before opening the combo (it could be anything)
                const char* comboText = modes[(int)mPreviewRenderSettings.mode];
                if (ImGui::BeginCombo("Rendering Mode", comboText)) {
                    for (int i = 0; i < IM_ARRAYSIZE(modes); i++) {
                        bool selected = ((int)mPreviewRenderSettings.mode == i);
                        if (ImGui::Selectable(modes[i], selected)) {
                            // Change material type
                            mPreviewRenderSettings.mode = (RenderMode)i;
                        }

                        if (selected) ImGui::SetItemDefaultFocus();
                    }
                    ImGui::EndCombo();
                }
                ImGui::Checkbox("Fast Mode", &mPreviewRenderSettings.fastMode);

                ImGui::NewLine();

                int* res = new int[2]{ (int)mOutputRenderSettings.resolution.x, (int)mOutputRenderSettings.resolution.y };
                ImGui::InputInt2("Resolution", res);
                if (res[0] > 0) mOutputRenderSettings.resolution.x = res[0];
                if (res[1] > 0) mOutputRenderSettings.resolution.y = res[1];

                ImGui::DragInt("Max Bounce Depth", &mOutputRenderSettings.maxDepth, 1, 1, 100);
                if (mOutputRenderSettings.maxDepth < 1) mOutputRenderSettings.maxDepth = 1;
                if (mOutputRenderSettings.maxDepth > 100) mOutputRenderSettings.maxDepth = 100;

                ImGui::DragInt("Samples Per Frame", &mOutputRenderSettings.samples, 1, 1, 10000);
                if (mOutputRenderSettings.samples < 1) mOutputRenderSettings.samples = 1;
                if (mOutputRenderSettings.samples > 10000) mOutputRenderSettings.samples = 10000;

                ImGui::DragInt("Threads", &mPreviewRenderSettings.numThreads, 1, 1, 30);
                if (mOutputRenderSettings.numThreads < 1) mOutputRenderSettings.numThreads = 1;
                if (mOutputRenderSettings.numThreads > 30) mOutputRenderSettings.numThreads = 30;

                ImGui::ColorEdit3("Ambient Light Colour", (float*)&mOutputRenderSettings.ambientLight);
                ImGui::Checkbox("Checkerboard", &mOutputRenderSettings.checkerboard);
                ImGui::Checkbox("Explicit Light Sampling", &mOutputRenderSettings.directLightSampling);
                ImGui::EndTabItem();
            }
            ImGui::EndTabBar();
        }

        ImGui::End();
    }

    void UIViewport() {
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

        mMouseInViewport = ImGui::IsWindowHovered();
        mViewportTopLeft = { textureTopLeftInWindow.x, textureTopLeftInWindow.y };
        mViewportBottomRight = { textureTopLeftInWindow.x + size.x * scale, textureTopLeftInWindow.y + size.y * scale };

        ImGui::End();
        ImGui::PopStyleVar();
    }

    void UIRayVisualization() {
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
        if (mRayVisualizationSettings.shortenedRayLength <= 0) mRayVisualizationSettings.shortenedRayLength = 0.01;
        if (mRayVisualizationSettings.shortenedRayLength > mRayVisualizationSettings.shortenedRayLength) mRayVisualizationSettings.shortenedRayLength = mRayVisualizationSettings.maxDistance;

        ImGui::PopItemWidth();

        ImGui::Checkbox("Update regularly", &mRayVisualizationSettings.updateRegularly);
        ImGui::SameLine();
        ImGui::SetCursorPos(ImVec2(160, ImGui::GetCursorPosY()));
        ImGui::PushItemWidth(ImGui::GetWindowWidth() * 0.65 - 160 + 10);
        ImGui::DragInt("Update Frames", &mRayVisualizationSettings.framesPerUpdate, 1, 1, 10);
        if (mRayVisualizationSettings.framesPerUpdate <= 0) mRayVisualizationSettings.framesPerUpdate = 1;
        if (mRayVisualizationSettings.framesPerUpdate > 10) mRayVisualizationSettings.shortenedRayLength = 10;

        ImGui::PopItemWidth();

        ImGui::DragInt("Initial rays cast", &mRayVisualizationSettings.initialRays, 1, 1, 100);
        if (mRayVisualizationSettings.initialRays <= 0) mRayVisualizationSettings.initialRays = 1;
        if (mRayVisualizationSettings.initialRays > 100) mRayVisualizationSettings.initialRays = 100;

        ImGui::DragInt("Max bounce depth", &mRayVisualizationSettings.maxBounceDepth, 1, 1, 100);
        if (mRayVisualizationSettings.maxBounceDepth <= 0) mRayVisualizationSettings.maxBounceDepth = 1;
        if (mRayVisualizationSettings.maxBounceDepth > 100) mRayVisualizationSettings.maxBounceDepth = 100;

        ImGui::DragFloat("Max distance", &mRayVisualizationSettings.maxDistance, 0.01, 0.01, 1000);
        if (mRayVisualizationSettings.framesPerUpdate <= 0) mRayVisualizationSettings.framesPerUpdate = 0.01;
        if (mRayVisualizationSettings.framesPerUpdate > 1000) mRayVisualizationSettings.shortenedRayLength = 1000;

        ImGui::Checkbox("Same colour", &mRayVisualizationSettings.sameColour);
        ImGui::SameLine();
        ImGui::SetCursorPos(ImVec2(160, ImGui::GetCursorPosY()));
        ImGui::PushItemWidth(ImGui::GetWindowWidth() * 0.65 - 160 + 10);
        ImGui::ColorEdit3("Colour", (float*)&mRayVisualizationSettings.lineColour);
        ImGui::PopItemWidth();

        if (ImGui::Button("Generate Rays")) GenerateVisualization();
        ImGui::End();
    }

    void UpdateImGui() {
        // ImGui stuff (from example)
        ImGui_ImplSDLRenderer_NewFrame();
        ImGui_ImplSDL2_NewFrame();
        ImGui::NewFrame();

        // Enable docking
        ImGui::DockSpaceOverViewport();

        UIMenuBar();
        UIFpsOverlay();
        UICamera();
        UIObjectSelect();
        UIObjectProperties();
        UIMaterialProperties();
        UIRenderSettings();
        UIViewport();
        UIRayVisualization();

        // Draw to the screen
        ImGui::Render();
        ImGui_ImplSDLRenderer_RenderDrawData(ImGui::GetDrawData());
    }

    void TrySelectObject() {
        int x, y;
        SDL_GetMouseState(&x, &y);
        if (x >= mViewportTopLeft.x && x <= mViewportBottomRight.x && y >= mViewportTopLeft.y && y <= mViewportBottomRight.y) {
            // Work out which object is being clicked and select it
            Vector2f relativePos = { (x - mViewportTopLeft.x) / (mViewportBottomRight.x - mViewportTopLeft.x), (y - mViewportTopLeft.y) / (mViewportBottomRight.y - mViewportTopLeft.y) };
            Vector3f viewportPos = mScene.camera.GetViewportPos(relativePos);
            Ray ray = Ray(mScene.camera.GetPosition(), viewportPos);
            RayPayload payload;
            if (mScene.ClosestHit(ray, 0.001, FLT_MAX, payload)) {
                mSelectedObject = mScene.GetObjectID(payload.object);
                if (mLookAtSelected) mRedrawThisFrame = true;
            }
            else {
                // De-select
                mSelectedObject = -1;
            }
        }
    }

    void DrawRay(Texture* image, Line2D line, Line3D line3d) {
        // Use the Bresenham algorithm to draw the ray onto the screen.

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

                // Make sure the pixel is on screen.
                if (xCoord >= 0 && xCoord < image->GetWidth() && yCoord >= 0 && yCoord < image->GetHeight()) {

                    // Get the position of the line through that pixel.
                    Vector3f toLine = mScene.camera.GetViewportPos({ (float)xCoord, (float)yCoord });
                    toLine.Normalize();
                    float t = (Vector2f((float)xCoord, (float)yCoord) - line.start).Magnitude() / (line.end - line.start).Magnitude();
                    Vector3f pos = line3d.start + t * (line3d.end - line3d.start);

                    // Only draw if the line is in front of every other object.
                    if ((pos - mScene.camera.GetPosition()).Magnitude() < mDepthBufferOld[xCoord][yCoord] || mDepthBufferOld[xCoord][yCoord] < 0) {
                        // Alpha blend between the line colour and the image colour.
                        colour = line.colour * alpha + image->GetColourAt({ (float)xCoord, (float)yCoord }) / 255 * (1 - alpha);
                        // Multiply by the image colour if the object is glass.
                        if (mDepthBufferOld[xCoord][yCoord] < 0 && (pos - mScene.camera.GetPosition()).Magnitude() > abs(mDepthBufferOld[xCoord][yCoord]))
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

                    if ((pos - mScene.camera.GetPosition()).Magnitude() < mDepthBufferOld[xCoord][yCoord] || mDepthBufferOld[xCoord][yCoord] < 0) {
                        colour = line.colour * alpha + image->GetColourAt({ (float)xCoord, (float)yCoord }) / 255 * (1 - alpha);
                        if (mDepthBufferOld[xCoord][yCoord] < 0 && (pos - mScene.camera.GetPosition()).Magnitude() > abs(mDepthBufferOld[xCoord][yCoord]))
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
                    Ray ray = Ray(mScene.camera.GetPosition(), viewportPos);
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

    void ConvertRenderedImageToTexture(Colour** image, Texture* texture, RenderSettings settings) {
        texture->Lock();
        for (int y = 0; y < settings.resolution.y; y++) {
            for (int x = 0; x < settings.resolution.x; x++) {
                Colour colour = Vector3f();

                // If checkerboard rendering is enabled, fill in the empty squares.
                if (settings.checkerboard && !((x % 2 == 0 && y % 2 == 0) || (x % 2 != 0 && y % 2 != 0))) {
                    int samples = 0;
                    if (x - 1 >= 0) {
                        colour += image[x - 1][y];
                        samples++;
                    }
                    if (x + 1 < settings.resolution.x) {
                        colour += image[x + 1][y];
                        samples++;
                    }
                    if (y - 1 >= 0) {
                        colour += image[x][y - 1];
                        samples++;
                    }
                    if (y + 1 < settings.resolution.y) {
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
                                    texture->SetColourAt({ (float)xOffset, (float)yOffset }, { 255, 50, 0 });
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
            if (event.type == SDL_MOUSEBUTTONDOWN && event.button.button == SDL_BUTTON_LEFT) {
                if (mMouseInViewport) TrySelectObject();
            }
            if (event.type == SDL_MOUSEBUTTONUP) {
                if (mMouseDrag) mMouseDrag = false;
            }
            if (event.type == SDL_MOUSEWHEEL && mMouseInViewport) {
                if (event.wheel.y > 0) mScene.camera.MoveInDirection({ 0, 0, 0.6 });
                else mScene.camera.MoveInDirection({ 0, 0, -0.8 });
                mRedrawThisFrame = true;
            }
            if (event.type == SDL_MOUSEBUTTONDOWN && event.button.button == SDL_BUTTON_MIDDLE && mMouseInViewport) {
                int x, y;
                SDL_GetMouseState(&x, &y);
                mMousePos = { (float)x, (float)y };
            }
            if (event.type == SDL_MOUSEMOTION && event.button.button == SDL_BUTTON_MIDDLE) {
                if ((!mMouseDrag && mMouseInViewport) || mMouseDrag) {
                    mMouseDrag = true;
                    int x, y;
                    SDL_GetMouseState(&x, &y);
                    Vector3f transform;
                    transform.x = mMousePos.x - x;
                    transform.y = y - mMousePos.y;
                    mScene.camera.MoveInDirection(transform * 0.02);
                    mMousePos = { (float)x, (float)y };
                    mRedrawThisFrame = true;
                }
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

        else if (keyboardState[SDL_SCANCODE_W]) {
            mScene.camera.MoveInDirection({ 0, 0, 0.6 });
            mRedrawThisFrame = true;
        }
        else if (keyboardState[SDL_SCANCODE_S]) {
            mScene.camera.MoveInDirection({ 0, 0, -0.6 });
            mRedrawThisFrame = true;
        }
        else if (keyboardState[SDL_SCANCODE_A]) {
            mScene.camera.MoveInDirection({ -0.3, 0, 0 });
            mRedrawThisFrame = true;
        }
        else if (keyboardState[SDL_SCANCODE_D]) {
            mScene.camera.MoveInDirection({ 0.3, 0, 0 });
            mRedrawThisFrame = true;
        }

    }

    void DrawRayVisualization() {
        for each (Line3D line in mRayVisualizationLines) {
            // Make sure line is visible.
            if ((line.start - mScene.camera.GetPosition()).Dot(mScene.camera.GetDirection()) <= 0 || (line.end - mScene.camera.GetPosition()).Dot(mScene.camera.GetDirection()) <= 0) continue;

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

    void RenderToImage() {
        float aspectRatio = mOutputRenderSettings.resolution.x / mOutputRenderSettings.resolution.y;
        mScene.camera.SetAspectRatio(aspectRatio);

        Colour** output = new Colour * [mOutputRenderSettings.resolution.x];
        float** outputDepth = new float * [mOutputRenderSettings.resolution.x];
        for (int i = 0; i < mOutputRenderSettings.resolution.x; i++) {
            output[i] = new Colour[mOutputRenderSettings.resolution.y];
            outputDepth[i] = new float[mOutputRenderSettings.resolution.y];
        }
        Texture* outputTexture = new Texture(mOutputRenderSettings.resolution.x, mOutputRenderSettings.resolution.y);
        
        SDL_SetTextureScaleMode(outputTexture->GetRawTexture(), SDL_ScaleModeBest);

        mScene.UpdateObjects();

        output = GetRenderer()->RenderScene(mScene, output, outputDepth, mOutputRenderSettings, 1);
        ConvertRenderedImageToTexture(output, outputTexture, mOutputRenderSettings);

        outputTexture->SaveToFile((mOutputImageDirectory + "\\" + mOutputImageName + ".bmp").c_str());

#ifdef WINDOWS
        if (mOpenDirectoryOnComplete) ShellExecuteA(NULL, "open", mOutputImageDirectory.c_str(), NULL, NULL, SW_SHOWDEFAULT);
        if (mOpenFileOnComplete)      ShellExecuteA(NULL, "open", (mOutputImageDirectory + "\\" + mOutputImageName + ".bmp").c_str(), NULL, NULL, SW_SHOWDEFAULT);
#endif // WINDOWS

        mScene.camera.SetAspectRatio(mAspectRatio);
    }

    void Update(float deltaTime) {
        // Update the window title to show the name of the scene
        std::string title = "Ray Tracing Optics Simulator: ";
        if (mScene.GetName() != "default") title += mScene.GetName();
        else title += "unnamed";
        // Add a * if the file has unsaved changes.
        if (mScene.IsModified() || mScene.GetName() == "default") title += "*";
        GetWindow()->SetTitle(title.c_str());

        mFrame++;

        // Point camera at selected object.
        if (mLookAtSelected && mSelectedObject >= 0) {
            if (mScene.GetObjects()[mSelectedObject]->name != "Ground") mScene.camera.LookAt(mScene.GetObjects()[mSelectedObject]->GetPosition());
        }

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

        ConvertRenderedImageToTexture(mRenderedImage, mOutputTexture, mPreviewRenderSettings);
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

    Camera     mDefaultCamera;

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
    RenderSettings mOutputRenderSettings;

    std::string   mOutputImageName;
    std::string   mOutputImageDirectory;
    bool          mOpenDirectoryOnComplete;
    bool          mOpenFileOnComplete;

    std::string   mNewObjectName; // Data for creating a new object.
    int           mNewObjectType;
    std::string   mNewObjectPath;
    int           mSelectedObject;

    bool       mLookAtSelected;

    bool       mMouseInViewport;
    Vector2f   mViewportTopLeft;
    Vector2f   mViewportBottomRight;

    bool       mMouseDrag;
    Vector2f   mMousePos;

    RayVisualizationSettings  mRayVisualizationSettings;
    std::vector<Line3D>       mRayVisualizationLines;

    ImFont*    mMonoFont;
};

int main(int argc, char* argv[]) {
    Application* app = new FinalApp(argv[1]);
    app->Run();
    return 0;
}