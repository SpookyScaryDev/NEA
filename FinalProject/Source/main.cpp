#include "imgui.h"
#include "imgui_impl_sdl.h"
#include "imgui_impl_sdlrenderer.h"
#include <stdio.h>
#include <iostream>
#include <SDL.h>

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

using namespace Prototype;

class PrototypeApp : public Application {
public:
    PrototypeApp() : Application("Ray Tracing Optics Simulator", 1100, 720) {
        SetUpImGui();

        width = 300;
        height = 225;
        aspectRatio = (float)width / (float)height;

        // Need to use an array as texture data wraps around after 255!
        image = new Colour * [width];
        isSelectedObjectVisible = new bool * [width];
        for (int i = 0; i < width; i++) {
            image[i] = new Colour[height];
            isSelectedObjectVisible[i] = new bool[height];
        }

        renderSettings = RenderSettings();
        renderSettings.resolution = { (float)width, (float)height };
        renderSettings.maxDepth = 10;
        renderSettings.samples = 1;
        renderSettings.ambientLight = Vector3f(1, 1, 1);
        renderSettings.checkerboard = false;

        ambientLightColour = { renderSettings.ambientLight.x, renderSettings.ambientLight.y, renderSettings.ambientLight.z, 1 };

        redraw = false;

        // The image which will be rendered on the GUI.
        finalImage = new Texture(width, height);

        SDL_SetTextureScaleMode(finalImage->GetRawTexture(), SDL_ScaleModeBest);

        selectedObject = 0;

        GenerateScene();
    }

    void GenerateScene() {
        // Generate a test scene.
        srand(2);

        Material ground = Material(MaterialType::Lambertian, { 0.5, 0.5, 0.5 });
        scene.AddObject("Ground", (Object*) new Sphere({ 0, -5000, -2 }, 5000, ground));

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

        scene.AddObject("Cube", (Object*)new Mesh({ -0.46, 0.09, 0-0.2 }, "cube.obj", metal));

        Camera camera = Camera(aspectRatio, 1.2, { 0,  0.13, 0.3 });

        scene.SetCamera(camera);
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
        ImGui::StyleColorsDark();
        //ImGui::StyleColorsClassic();

        // Setup Platform/Renderer backends
        ImGui_ImplSDL2_InitForSDLRenderer(GetWindow()->GetRawWindow(), GetRenderer()->GetRawRenderer());
        ImGui_ImplSDLRenderer_Init(GetRenderer()->GetRawRenderer());

        io.Fonts->AddFontFromFileTTF("Roboto-Regular.ttf", 15.0f);

        SetImGuiStyle(false);
    }

    void SetImGuiStyle(bool darkMode) {
        ImGuiStyle& style = ImGui::GetStyle();

        style.FrameRounding = 3.0f;
        style.WindowBorderSize = 0;

        if (darkMode) {
            ImGui::StyleColorsDark();
        }
        else {
            ImGui::StyleColorsLight();
        }

        for (int i = 0; i <= ImGuiCol_COUNT; i++) {
            ImGuiCol_ colourID = (ImGuiCol_)i;
            ImVec4& colour = style.Colors[i];
            // Make window and title bar background lighter.
            if (darkMode && (colourID == ImGuiCol_WindowBg || colourID == ImGuiCol_TitleBg)) {
                colour.x = 3 * colour.x;
                colour.y = 3 * colour.y;
                colour.z = 3 * colour.z;
            }
            // Make backgrounds darker. If statement condition taken from https://gist.github.com/dougbinks/8089b4bbaccaaf6fa204236978d165a9.
            if (!darkMode && (colourID != ImGuiCol_ModalWindowDimBg) &&
                (colourID != ImGuiCol_NavWindowingDimBg) &&
                ((colourID == ImGuiCol_FrameBg) ||
                    (colourID == ImGuiCol_WindowBg) ||
                    (colourID == ImGuiCol_ChildBg))) {
                colour.x = 0.9 * colour.x;
                colour.y = 0.9 * colour.y;
                colour.z = 0.9 * colour.z;
            }
        }
    }

    void UpdateImGui() {
        ImGui_ImplSDLRenderer_NewFrame();
        ImGui_ImplSDL2_NewFrame();
        ImGui::NewFrame();

        ImGui::DockSpaceOverViewport();

        // Menu bar.
        {
            if (ImGui::BeginMainMenuBar()) {
                if (ImGui::BeginMenu("File")) {
                    ImGui::MenuItem("New");
                    ImGui::MenuItem("Save");
                    ImGui::MenuItem("Save As");
                    ImGui::MenuItem("Open");
                    ImGui::EndMenu();
                }

                ImGui::EndMainMenuBar();
            }
        }

        // FPS overlay
        {
            ImGui::SetNextWindowBgAlpha(0.35f);
            ImGui::Begin("Example: Simple overlay", (bool*)1, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoDocking);
            //ImGui::Text("Renderer API: direct 3d");
            ImGui::Text("%.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
            ImGui::Text("Renderer: %.3f ms/frame (%.1f FPS)", rendererFps * 1000, 1000/(rendererFps * 1000));
            ImGui::End();
        }

        // Camera position
        {
            ImGui::Begin("Camera", (bool*)1);
            redraw |= ImGui::DragFloat3("Position", (float*)&scene.camera.position, 0.01);
            //ImGui::DragFloat3("Rotation", (float*)&obj->rotation, 0.5);

            ImGui::End();
        }

        // Object selection window.
        {
            ImGui::Begin("Objects", (bool*)1);
            if (ImGui::Button("Add New Object", { ImGui::GetWindowWidth(), 20 })) {
                //scene.AddObject(GenerateRandomObject());
                ImGui::OpenPopup("Add Object");
            }

            if (ImGui::BeginPopup("Add Object")) {
                if (ImGui::Button("Ok")) {
                    ImGui::CloseCurrentPopup();
                }
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
                sprintf(buf, (std::string("##object_id_") + std::to_string(obj->id)).c_str(), n + 1);
                if (ImGui::Selectable(buf, selectedObject == n))
                    selectedObject = n; // Set currently selected object.
                ImGui::SameLine();
                ImGui::Text(obj->name.c_str());
            }

            ImGui::End();
        }

        // Pannel to edit currently selected object.
        {
            ImGui::Begin("Object Properties", (bool*)1);
            Object* obj = scene.GetObjects()[selectedObject];

            char buffer[32];
            sprintf_s(buffer, obj->name.c_str(), obj->name.length());

            ImGui::InputText("Name", buffer, 100);
            redraw |= ImGui::DragFloat3("Position", (float*)&obj->position, 0.01);
            redraw |= ImGui::DragFloat("Scale", &obj->scale, 0.01, 0.01, 100);
            //ImGui::DragFloat3("Rotation", (float*)&obj->rotation, 0.5);

            obj->name = std::string(buffer);

            ImGui::End();
        }

        // Pannel to edit the material of the currently selected object.
        {
            ImGui::Begin("Material Editor", (bool*)1);

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
            if (material->materialType != MaterialType::Lambertian) redraw |= ImGui::SliderFloat("Roughness", &material->roughness, 0, 1);

            ImGui::End();
        }

        // Pannel to set the renderer configuration.
        {
            ImGui::Begin("Render Settings", (bool*)1);
            if (ImGui::BeginTabBar("Render Settings Tab Bar")) {
                if (ImGui::BeginTabItem("Preview")) {
                    redraw |= ImGui::SliderInt("Max Depth", &renderSettings.maxDepth, 1, 100);
                    redraw |= ImGui::SliderInt("Samples Per Frame", &renderSettings.samples, 1, 100);
                    redraw |= ImGui::ColorEdit3("Ambient Light Colour", (float*)&renderSettings.ambientLight);
                    redraw |= ImGui::Checkbox("Checkerboard", &renderSettings.checkerboard);
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
            ImGui::Begin("Scene", (bool*)1);
            ImGui::SetItemAllowOverlap();

            SDL_Point size;
            SDL_QueryTexture(finalImage->GetRawTexture(), NULL, NULL, &size.x, &size.y);

            float scale = (ImGui::GetWindowWidth() - 15) / size.x;
            if (scale * size.y > ImGui::GetWindowHeight()) scale = (ImGui::GetWindowHeight() - 15) / size.y;

            //ImGui::SetCursorPos({ 0, 25 });
            ImVec2 imageStartScreen = ImGui::GetCursorScreenPos();
            //ImGui::GetWindowDrawList()->AddImage(
            //    (void*)finalImage->GetRawTexture(),
            //    ImVec2(ImGui::GetCursorScreenPos()),
            //    ImVec2((int)(ImGui::GetCursorScreenPos().x + size.x * scale), (int)(ImGui::GetCursorScreenPos().y + size.y * scale)));
            ImGui::Image((void*)(intptr_t)finalImage->GetRawTexture(), ImVec2(size.x * scale, size.y * scale));

            if (ImGui::IsMouseClicked(0) && ImGui::IsMouseHoveringRect(imageStartScreen, { imageStartScreen.x + size.x * scale, imageStartScreen.y + size.y * scale })) {
                ImVec2 pos = ImGui::GetMousePos();
                Vector2f relativePos = { (pos.x - imageStartScreen.x) / (scale * size.x), (pos.y - imageStartScreen.y) / (scale * size.y) };
                Vector3f viewportPos = scene.camera.GetViewportPos(relativePos);
                Ray ray = Ray(scene.camera.position, viewportPos);
                RayPayload payload;
                if (scene.ClosestHit(ray, 0.001, FLT_MAX, payload)) {
                    selectedObject = payload.object->id;
                }
            }

            ImGui::End();
        }

        ImGui::Render();
        ImGui_ImplSDLRenderer_RenderDrawData(ImGui::GetDrawData());

    }

    void Update(float deltaTime) {
        // Start rendering again from scratch if settings are changed.
        if (redraw) frame = 1;
        redraw = false;
        frame++;

        std::chrono::system_clock::time_point previousTime = std::chrono::system_clock::now();
        image = GetRenderer()->RenderScene(scene, image, renderSettings, frame);
        std::chrono::system_clock::time_point currentTime = std::chrono::system_clock::now();
        std::chrono::duration<float> elapsed = std::chrono::duration_cast<std::chrono::duration<float>>(currentTime - previousTime);
        rendererFps = elapsed.count();

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

                Vector2f screenPos = { x / renderSettings.resolution.x, y / renderSettings.resolution.y };
                Vector3f viewportPos = scene.camera.GetViewportPos(screenPos);
                Ray ray = Ray(scene.camera.position, viewportPos);
                Object* selected = scene.GetObjects()[selectedObject];
                RayPayload payload;
                isSelectedObjectVisible[x][y] = selected->Intersect(ray, 0, FLT_MAX, payload);

            }
        }
        for (int y = 0; y < renderSettings.resolution.y; y++) {
            for (int x = 0; x < renderSettings.resolution.x; x++) {
                if (isSelectedObjectVisible[x][y]) {
                    for (int xOffset = x - 1; xOffset <= x + 1; xOffset++) {
                        for (int yOffset = y - 1; yOffset <= y + 1; yOffset++) {
                            if (xOffset >= 0 && xOffset < renderSettings.resolution.x && yOffset >= 0 && yOffset < renderSettings.resolution.y) {
                                if (!isSelectedObjectVisible[xOffset][yOffset]) {
                                    finalImage->SetColourAt({ (float)xOffset, (float)yOffset }, { 255, 50, 0 });
                                }
                            }
                        }
                    }
                }
            }
        }
        finalImage->Unlock();

        // Process input for ImGui.
        SDL_Event event;
        while (SDL_PollEvent(&event))
        {
            ImGui_ImplSDL2_ProcessEvent(&event);
            if (event.type == SDL_QUIT)
                mIsRunning = false;
            if (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_CLOSE && event.window.windowID == SDL_GetWindowID(GetWindow()->GetRawWindow()))
                mIsRunning = false;
        }

        // Draw the interface.
        GetRenderer()->Clear();
        UpdateImGui();
    }

private:
    Colour** image;       // The image generated by the renderer.
    bool** isSelectedObjectVisible;
    Texture* finalImage;  // The image which will be drawn.

    RenderSettings renderSettings;

    Scene scene;

    int width;
    int height;
    float aspectRatio;

    int frame = 0;        // Frame referes to the number of frames that nothing has changed, used for temporal anti-aliasing.

    // UI state:
    int selectedObject;

    bool redraw;
    ImVec4 ambientLightColour;

    float rendererFps;
};

int main(int, char**) {

    Application* app = new PrototypeApp();
    app->Run();

    return 0;
}
