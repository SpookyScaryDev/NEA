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

#include <Error.h>

using namespace Prototype;

class PrototypeApp : public Application {
public:
    PrototypeApp() : Application("Prototype", 1100, 720) {
        SetUpImGui();

        width = 300;
        height = 225;
        aspectRatio = (float)width / (float)height;

        // Need to use an array as texture data wraps around after 255!
        image = new Colour *[width];
        for (int i = 0; i < width; i++) {
            image[i] = new Colour[height];
        }

        renderSettings = RenderSettings();
        renderSettings.resolution = { (float)width, (float)height };
        renderSettings.maxDepth = 10;
        renderSettings.samples = 1;
        renderSettings.ambientLight = Vector3f(1, 1, 1);

        ambientLightColour = { renderSettings.ambientLight.x, renderSettings.ambientLight.y, renderSettings.ambientLight.z, 1 };

        redraw = false;

        // The image which will be rendered on the GUI.
        finalImage = new Texture(width, height);

        GenerateScene();
    }

    void GenerateScene() {
        // Generate a test scene.
        srand(2);
        for (int i = -1; i <= 1; i += 1) {
            for (int j = -1; j <= 1; j += 1) {
                Material mat;
                switch (rand() % 4) {
                case 0:
                    mat = Material(MaterialType::Lambertian, { rand() / (RAND_MAX + 1.0f) * 0.9f + 0.1f , rand() / (RAND_MAX + 1.0f) * 0.9f + 0.1f, rand() / (RAND_MAX + 1.0f) * 0.9f + 0.1f } );
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
                    scene.AddObject((Object*) new Sphere({ i * 0.7f, yPos, j * 0.7f - 2 }, size, mat));
            }
        }

        Material ground = Material(MaterialType::Lambertian, { 0.5, 0.5, 0.5 });
        Material metal  = Material(MaterialType::Glossy,     { 0.8, 0.8, 0.8 }, 0);
        Material glass  = Material(MaterialType::Glass,      { 1.0, 1.0, 1.0 }, 0, 1.44);

        Material light = Material(MaterialType::Lambertian, { 1, 1, 1 }, 1, 1, { 100, 100, 100 });

        scene.AddObject((Object*)new Sphere({ 1.6, 0.35, -2 }, 0.35, metal));
        scene.AddObject((Object*)new Sphere({ 0, 0.375, -2 }, -0.375, glass));

        scene.AddObject((Object*)new Sphere({   0,   1.5, -2 },    0.15, light));

        Camera camera = Camera(aspectRatio, 1.2, { 0.1,  0.4, -0.1 });

        scene.SetCamera(camera);
        scene.AddObject((Object*) new Sphere({ 0, -5000, -2 }, 5000, ground));
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
    }

    void UpdateImGui() {
        ImGui_ImplSDLRenderer_NewFrame();
        ImGui_ImplSDL2_NewFrame();
        ImGui::NewFrame();

        ImGui::DockSpaceOverViewport();

        {
            ImGui::Begin("Objects");
            ImGui::End();
        }

        {
            ImGui::Begin("Renderer Settings");
            ImGui::PushItemWidth(ImGui::GetWindowWidth() * 0.5f);

            if (ImGui::CollapsingHeader("Preview Render Settings", 32)) {
                redraw |= ImGui::SliderInt("Max Depth", &renderSettings.maxDepth, 1, 100);
                redraw |= ImGui::SliderInt("Samples Per Frame", &renderSettings.samples, 1, 100);
                redraw |= ImGui::ColorEdit3("Ambient Light Colour", (float*)&ambientLightColour);

                renderSettings.ambientLight = Vector3f(ambientLightColour.x, ambientLightColour.y, ambientLightColour.z);
            }

            ImGui::End();
        }

        {
            ImGui::Begin("Scene");
            float scale = 2;

            SDL_Point size;
            SDL_QueryTexture(finalImage->GetRawTexture(), NULL, NULL, &size.x, &size.y);

            ImGui::GetWindowDrawList()->AddImage(
                (void*)finalImage->GetRawTexture(),
                ImVec2(ImGui::GetCursorScreenPos()),
                ImVec2((int)(ImGui::GetCursorScreenPos().x + size.x * scale), (int)(ImGui::GetCursorScreenPos().y + size.y * scale)));

            ImGui::SetCursorPosY((size.y * scale + 50));
            ImGui::Text("%.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);

            ImGui::End();
        }

        ImGui::Render();
        ImGui_ImplSDLRenderer_RenderDrawData(ImGui::GetDrawData());

    }

    void Update(float deltaTime) {
        if (redraw) frame = 0;
        redraw = false;
        frame++;

        image = GetRenderer()->RenderScene(scene, image, renderSettings, frame);

        // Prepare image texture for rendering.
        finalImage->Lock();
        for (int y = 0; y < renderSettings.resolution.y; y++) {
            for (int x = 0; x < renderSettings.resolution.x; x++) {
                Colour colour = image[x][y];
                finalImage->SetColourAt({ (float)x, (float)y }, colour * 255);
            }
        }
        finalImage->Unlock();

        SDL_Event event;
        while (SDL_PollEvent(&event))
        {
            ImGui_ImplSDL2_ProcessEvent(&event);
            if (event.type == SDL_QUIT)
                mIsRunning = false;
            if (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_CLOSE && event.window.windowID == SDL_GetWindowID(GetWindow()->GetRawWindow()))
                mIsRunning = false;
        }

        GetRenderer()->Clear();
        UpdateImGui();
    }

private:
    Colour** image;       // The image generated by the renderer.
    Texture* finalImage;  // The image which will be drawn.

    RenderSettings renderSettings;

    Scene scene;

    int width;
    int height;
    float aspectRatio;

    int frame = 0;        // Frame referes to the number of frames that nothing has changed, used for temporal anti-aliasing.

    bool redraw;
    ImVec4 ambientLightColour;
};

int main(int, char**) {

    Application* app = new PrototypeApp();
    app->Run();

    return 0;
}
