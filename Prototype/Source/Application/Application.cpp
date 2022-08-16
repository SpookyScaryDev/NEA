#include "Application.h"

#include <Window/Window.h>
#include <Renderer/Renderer.h>
#include <SDL.h>
#include <chrono>

namespace Prototype {

Application* Application::mApplication = nullptr;

Application::Application(const char* name, int width, int height) {
    // Initialize singleton.
    if (mApplication != nullptr) {
        //JEM_CORE_ERROR("Application already exists. You can only create 1 Application");
    }
    mApplication = this;
    mIsRunning = true;

    mWindow = new Window();
    mRenderer = new Renderer();

    Init(name, width, height);
}

Application::~Application() {
    Shutdown();
    mApplication = nullptr;
    delete mWindow;
}

Application* Application::GetApp() {
    return mApplication;
}

void Application::Init(const char* name, int width, int height) {
    printf("************************************************************\n");
    printf("Initializing Prototype:\n");
    printf("************************************************************\n");
    printf("Application::Init: Initializing Subsystems\n");

    printf("SDL_Init: Initializing SDL2\n");
    if (SDL_Init(SDL_INIT_EVERYTHING) != 0) {
        // JEM_CORE_ERROR("SDL_Init: Failed to initialize SDL2 - ", SDL_GetError());
    }

    printf("InitWindow: Creating Window\n");
    mWindow->Init(name, width, height);

    printf("Renderer::Init: Initializing Renderer\n");
    mRenderer->Init(mWindow);

    printf("************************************************************\n");
}

void Application::Shutdown() const{
    printf("Shutting down engine.");
    mRenderer->Shutdown();
    mWindow->Destroy();

    SDL_Quit();
}

Window* Application::GetWindow() const {
    return mWindow;
}

Renderer* Application::GetRenderer() const {
    return mRenderer;
}

void Application::Run() {
    while (mIsRunning) {
        printf("Running Application");

        // Get time.
        std::chrono::system_clock::time_point previousTime = std::chrono::system_clock::now();
        while (mIsRunning) {
            // Calculate elapsed time.
            std::chrono::system_clock::time_point currentTime = std::chrono::system_clock::now();
            std::chrono::duration<float> elapsed = std::chrono::duration_cast<std::chrono::duration<float>>(currentTime - previousTime);
            previousTime = currentTime;

            // Update using delta time.
            Update(elapsed.count());

            // Refresh Renderer.
            mRenderer->Refresh();
        };
    }
}

}
