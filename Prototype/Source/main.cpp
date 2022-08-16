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

using namespace Prototype;

class PrototypeApp : public Application {
public:
    PrototypeApp() : Application("Prototype", 1100, 720) {
        SetUpImGui();

        GetRenderer()->SetClearColour({ 50, 50, 50 });

        width = 500;
        height = 500;
        aspectRatio = (float)width / (float)height;
        image = new Texture(width, height);

        Camera camera = Camera(aspectRatio, 1, { 0, 0, 0 });
    }

    void SetUpImGui() {
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

        // Load Fonts
        // - If no fonts are loaded, dear imgui will use the default font. You can also load multiple fonts and use ImGui::PushFont()/PopFont() to select them.
        // - AddFontFromFileTTF() will return the ImFont* so you can store it if you need to select the font among multiple.
        // - If the file cannot be loaded, the function will return NULL. Please handle those errors in your application (e.g. use an assertion, or display an error and quit).
        // - The fonts will be rasterized at a given size (w/ oversampling) and stored into a texture when calling ImFontAtlas::Build()/GetTexDataAsXXXX(), which ImGui_ImplXXXX_NewFrame below will call.
        // - Read 'docs/FONTS.md' for more instructions and details.
        // - Remember that in C/C++ if you want to include a backslash \ in a string literal you need to write a float backslash \\ !
        //io.Fonts->AddFontDefault();
        //io.Fonts->AddFontFromFileTTF("../../misc/fonts/Roboto-Medium.ttf", 16.0f);
        //io.Fonts->AddFontFromFileTTF("../../misc/fonts/Cousine-Regular.ttf", 15.0f);
        //io.Fonts->AddFontFromFileTTF("../../misc/fonts/DroidSans.ttf", 16.0f);
        //io.Fonts->AddFontFromFileTTF("../../misc/fonts/ProggyTiny.ttf", 10.0f);
        //ImFont* font = io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\ArialUni.ttf", 18.0f, NULL, io.Fonts->GetGlyphRangesJapanese());
        //IM_ASSERT(font != NULL);
    }

    void UpdateImGui() {
        // Start the Dear ImGui frame
        ImGui_ImplSDLRenderer_NewFrame();
        ImGui_ImplSDL2_NewFrame();
        ImGui::NewFrame();

        ImGui::DockSpaceOverViewport();

        {
            ImGui::Begin("Objects");
            ImGui::End();
        }

        {
            ImGui::Begin("Scene");
            float scale = 1;

            SDL_Point size;
            SDL_QueryTexture(finalImage->GetRawTexture(), NULL, NULL, &size.x, &size.y);

            ImGui::GetWindowDrawList()->AddImage(
                (void*)finalImage->GetRawTexture(),
                ImVec2(ImGui::GetCursorScreenPos()),
                ImVec2((int)(ImGui::GetCursorScreenPos().x + size.x * scale), (int)(ImGui::GetCursorScreenPos().y + size.y * scale)), ImVec2(1, 0), ImVec2(0, 1));

            ImGui::SetCursorPosY((size.y * scale + 50));
            ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);

            ImGui::End();
        }

        ImGui::Render();
        ImGui_ImplSDLRenderer_RenderDrawData(ImGui::GetDrawData());

    }

    void Update(float deltaTime) {

        finalImage = new Texture(width, height);
        finalImage = GetRenderer()->RenderScene(finalImage);
        SDL_SetTextureScaleMode(finalImage->GetRawTexture(), SDL_ScaleMode::SDL_ScaleModeLinear);

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


        delete finalImage;

    }

private:
    Texture* image;
    Texture* finalImage;

    int width;
    int height;
    float aspectRatio;
};

int main(int, char**) {

    Application* app = new PrototypeApp();
    app->Run();

    return 0;
}
