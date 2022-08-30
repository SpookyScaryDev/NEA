// Dear ImGui: standalone example application for SDL2 + SDL_Renderer
// (SDL is a cross-platform general purpose library for handling windows, inputs, OpenGL/Vulkan/Metal graphics context creation, etc.)
// If you are new to Dear ImGui, read documentation from the docs/ folder + read the top of imgui.cpp.
// Read online: https://github.com/ocornut/imgui/tree/master/docs

// Important to understand: SDL_Renderer is an _optional_ component of SDL. We do not recommend you use SDL_Renderer
// because it provide a rather limited API to the end-user. We provide this backend for the sake of completeness.
// For a multi-platform app consider using e.g. SDL+DirectX on Windows and SDL+OpenGL on Linux/OSX.

#include "imgui.h"
#include "imgui_impl_sdl.h"
#include "imgui_impl_sdlrenderer.h"
#include <stdio.h>
#include <SDL.h>

#if !SDL_VERSION_ATLEAST(2,0,17)
#error This backend requires SDL 2.0.17+ because of SDL_RenderGeometry() function
#endif

void SetupImGuiStyle(bool bStyleDark_, float alpha_)
{
    ImGuiStyle& style = ImGui::GetStyle();

    if (bStyleDark_)
    {
        ImGui::StyleColorsDark();
    }
    else
    {
        ImGui::StyleColorsLight();
    }

    for (int i = 0; i <= ImGuiCol_COUNT; i++)
    {
        ImGuiCol_ ei = (ImGuiCol_)i;
        ImVec4& col = style.Colors[i];
        if ((ImGuiCol_ModalWindowDimBg != ei) &&
            (ImGuiCol_NavWindowingDimBg != ei) &&
            (col.w < 1.00f || (ImGuiCol_FrameBg == ei)
                || (ImGuiCol_WindowBg == ei)
                || (ImGuiCol_ChildBg == ei)))
        {
            col.x = alpha_ * col.x;
            col.y = alpha_ * col.y;
            col.z = alpha_ * col.z;
        }
    }

    style.ChildBorderSize = 1.0f;
    style.FrameBorderSize = 0.0f;
    style.PopupBorderSize = 1.0f;
    style.WindowBorderSize = 0.0f;
    style.FrameRounding = 3.0f;
    style.Alpha = 1.0f;
}

// Main code
int main(int, char**)
{
    // Setup SDL
    // (Some versions of SDL before <2.0.10 appears to have performance/stalling issues on a minority of Windows systems,
    // depending on whether SDL_INIT_GAMECONTROLLER is enabled or disabled.. updating to latest version of SDL is recommended!)
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_GAMECONTROLLER) != 0)
    {
        printf("Error: %s\n", SDL_GetError());
        return -1;
    }

    // Setup window
    SDL_WindowFlags window_flags = (SDL_WindowFlags)(SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);
    SDL_Window* window = SDL_CreateWindow("Ray Tracing NEA", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1100, 720, window_flags);

    // Setup SDL_Renderer instance
    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_PRESENTVSYNC | SDL_RENDERER_ACCELERATED);
    if (renderer == NULL)
    {
        SDL_Log("Error creating SDL_Renderer!");
        return false;
    }
    //SDL_RendererInfo info;
    //SDL_GetRendererInfo(renderer, &info);
    //SDL_Log("Current SDL_Renderer: %s", info.name);

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    //io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    //io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

    // Setup Dear ImGui style
    //ImGui::StyleColorsDark();
    //ImGui::StyleColorsClassic();

    // Setup Platform/Renderer backends
    ImGui_ImplSDL2_InitForSDLRenderer(window, renderer);
    ImGui_ImplSDLRenderer_Init(renderer);

    SetupImGuiStyle(false, 0.9);
    ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_DockingEnable;

    //ImGuiDockNode* Node = ImGui::DockBuilderGetNode(DockID);
    //Node->LocalFlags |= ImGuiDockNodeFlags_NoWindowMenuButton | ImGuiDockNodeFlags_NoCloseButton;

    // Load Fonts
    // - If no fonts are loaded, dear imgui will use the default font. You can also load multiple fonts and use ImGui::PushFont()/PopFont() to select them.
    // - AddFontFromFileTTF() will return the ImFont* so you can store it if you need to select the font among multiple.
    // - If the file cannot be loaded, the function will return NULL. Please handle those errors in your application (e.g. use an assertion, or display an error and quit).
    // - The fonts will be rasterized at a given size (w/ oversampling) and stored into a texture when calling ImFontAtlas::Build()/GetTexDataAsXXXX(), which ImGui_ImplXXXX_NewFrame below will call.
    // - Read 'docs/FONTS.md' for more instructions and details.
    // - Remember that in C/C++ if you want to include a backslash \ in a string literal you need to write a double backslash \\ !
    //io.Fonts->AddFontDefault();
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/Roboto-Medium.ttf", 16.0f);
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/Cousine-Regular.ttf", 15.0f);
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/DroidSans.ttf", 16.0f);
    io.Fonts->AddFontFromFileTTF("Roboto-Regular.ttf", 15.0f);
    //ImFont* font = io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\ArialUni.ttf", 18.0f, NULL, io.Fonts->GetGlyphRangesJapanese());
    //IM_ASSERT(font != NULL);

    // Our state
    int depth = 10;
    int spf = 1;
    ImVec4 col = ImVec4(1, 1, 1, 1);
    int selectedObj = 3;
    ImVec4 pos = ImVec4(0, 0, 0, 0);
    ImVec4 rot = ImVec4(0, 45, 0, 0);
    bool show = true;
    bool showMaterialDropdown = false;
    const char* materials[] = { "Lambertian", "Specular", "Glass" };
    int materialType = 1;
    ImVec4 colour = ImVec4(0.5, 0.8, 0.5, 0);
    float albedo = 0.5;
    float roughness = 0.2;

    SDL_Texture* scene =  SDL_CreateTextureFromSurface(renderer, SDL_LoadBMP("scene.bmp"));

    // Main loop
    bool done = false;
    while (!done)
    {
        // Poll and handle events (inputs, window resize, etc.)
        // You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants to use your inputs.
        // - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application, or clear/overwrite your copy of the mouse data.
        // - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application, or clear/overwrite your copy of the keyboard data.
        // Generally you may always pass all inputs to dear imgui, and hide them from your application based on those two flags.
        SDL_Event event;
        while (SDL_PollEvent(&event))
        {
            ImGui_ImplSDL2_ProcessEvent(&event);
            if (event.type == SDL_QUIT)
                done = true;
            if (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_CLOSE && event.window.windowID == SDL_GetWindowID(window))
                done = true;
        }

        // Start the Dear ImGui frame
        ImGui_ImplSDLRenderer_NewFrame();
        ImGui_ImplSDL2_NewFrame();
        ImGui::NewFrame();

        ImGui::DockSpaceOverViewport();

        ImGui::ShowDemoWindow();

        {
            if (ImGui::BeginMainMenuBar())
            {
                if (ImGui::BeginMenu("File"))
                {
                    if (ImGui::MenuItem("New"))
                    {
                        //Do something
                    }
                    ImGui::EndMenu();
                }
                if (ImGui::BeginMenu("Layout"))
                {
                    if (ImGui::MenuItem("New"))
                    {
                        //Do something
                    }
                    ImGui::EndMenu();
                }

                ImGui::EndMainMenuBar();
            }
        }

        {
            ImGui::SetNextWindowBgAlpha(0.35f);
            ImGui::Begin("Example: Simple overlay", (bool*)1, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoDocking);
            ImGui::Text("Renderer API: OpenGL");
            ImGui::Text("%.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
            ImGui::End();
        }

        {
            ImGui::Begin("Objects", (bool*)1);

            for (int n = 0; n < 5; n++)
            {
                ImGui::SetItemAllowOverlap();

                ImGui::SetCursorPos(ImVec2(0, n * 20 + 20));

                char buf[32];
                sprintf(buf, "##Show %d", n+1);
                
                ImGui::Checkbox(buf, &show);

                ImGui::SetCursorPos(ImVec2(25, n * 20 + 20 + 2));
                sprintf(buf, "Object %d", n+1);
                if (ImGui::Selectable(buf, selectedObj == n))
                    selectedObj = n;
            }

            ImGui::End();
        }

        {
            ImGui::Begin("Object Properties", (bool*)1);

            ImGui::InputText("Name", "Object 4", 100);
            ImGui::DragFloat3("Position", (float*)&pos, 0.01);
            ImGui::DragFloat3("Rotation", (float*)&rot, 0.5);

            ImGui::End();
        }

        {
            ImGui::Begin("Material Editor", (bool*)1);

            const char* combo_preview_value = materials[materialType];  // Pass in the preview value visible before opening the combo (it could be anything)
            if (ImGui::BeginCombo("Material Type", combo_preview_value))
            {
                for (int n = 0; n < IM_ARRAYSIZE(materials); n++)
                {
                    const bool is_selected = (materialType == n);
                    if (ImGui::Selectable(materials[n], is_selected))
                        materialType = n;

                    // Set the initial focus when opening the combo (scrolling + keyboard navigation focus)
                    if (is_selected)
                        ImGui::SetItemDefaultFocus();
                }
                ImGui::EndCombo();
            }

            ImGui::ColorEdit3("Colour", (float*)&colour);
            ImGui::SliderFloat("Albedo", &albedo, 0, 1);
            ImGui::SliderFloat("Roughness", &roughness, 0, 1);

            ImGui::End();
        }

        {
            ImGui::Begin("Renderer", (bool*)1);
            //ImGui::PushItemWidth(ImGui::GetWindowWidth() * 0.5f);
            ImGui::Text("Settings");
            if (ImGui::BeginTabBar("Render Settings")) {
                if (ImGui::BeginTabItem("Preview")) {
                    ImGui::SliderInt("Max Depth", &depth, 1, 100);
                    ImGui::SliderInt("Samples Per Frame", &spf, 1, 100);
                    ImGui::ColorEdit3("Ambient Light Colour", (float*)&col);
                    ImGui::EndTabItem();
                }
                if (ImGui::BeginTabItem("Output")) {
                    ImGui::SliderInt("Max Depth", &depth, 1, 100);
                    ImGui::SliderInt("Samples Per Frame", &spf, 1, 100);
                    ImGui::ColorEdit3("Ambient Light Colour", (float*)&col);
                    ImGui::EndTabItem();
                }
                ImGui::EndTabBar();
            }

            ImGui::End();
        }

        {
            ImGui::Begin("Scene", (bool*)1);
            //ImGui::SetCursorPos(ImVec2(0, 0));
            float scale = 1.1;

            SDL_Point size;
            SDL_QueryTexture(scene, NULL, NULL, &size.x, &size.y);

            ImGui::GetWindowDrawList()->AddImage(
                (void*)scene,
                ImVec2(ImGui::GetCursorScreenPos()),
                ImVec2((int)(ImGui::GetCursorScreenPos().x + size.x * scale), (int)(ImGui::GetCursorScreenPos().y + size.y * scale)));

            //ImGui::SetCursorPosY((size.y * scale + 50));
            //ImGui::Text("%.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);

            ImGui::End();
        }

        ImGui::Render();
        ImGui_ImplSDLRenderer_RenderDrawData(ImGui::GetDrawData());

        // Rendering
        ImGui::Render();
        //SDL_SetRenderDrawColor(renderer, (Uint8)(clear_color.x * 255), (Uint8)(clear_color.y * 255), (Uint8)(clear_color.z * 255), (Uint8)(clear_color.w * 255));
        SDL_RenderClear(renderer);
        ImGui_ImplSDLRenderer_RenderDrawData(ImGui::GetDrawData());
        SDL_RenderPresent(renderer);
    }

    // Cleanup
    ImGui_ImplSDLRenderer_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
