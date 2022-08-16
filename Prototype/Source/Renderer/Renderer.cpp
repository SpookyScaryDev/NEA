#include "Renderer.h"

#include <SDL.h>
#include <Window/Window.h>
#include <Maths/Vector2f.h>
#include <Maths/Vector3f.h>

#include "Sphere.h"
#include "Camera.h"

namespace Prototype {

Renderer::Renderer() {
    mRawRenderer = nullptr;
}

void Renderer::Init(Window* window) {
    mRawRenderer = SDL_CreateRenderer(window->GetRawWindow(),
        -1,
        SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC
    );

    if (mRawRenderer == nullptr) {
        // The renderer could not be created.
        //JEM_CORE_ERROR("Renderer::Init: Failed to create renderer - ", SDL_GetError());
    }

    SDL_SetRenderDrawBlendMode(mRawRenderer, SDL_BLENDMODE_BLEND);

    SDL_RendererInfo rendererInfo;
    SDL_GetRendererInfo(mRawRenderer, &rendererInfo);
    //JEM_CORE_MESSAGE("Renderer API: ", rendererInfo.name);

    mClearColour = Vector3f(0, 0, 0);
}

void Renderer::Shutdown() {
    SDL_DestroyRenderer(mRawRenderer);
    mRawRenderer = nullptr;
}

SDL_Renderer* Renderer::GetRawRenderer() {
    return mRawRenderer;
}

void Renderer::SetClearColour(const Colour& colour) {
    mClearColour = colour;
}

void Renderer::Clear() {
    SDL_SetRenderDrawColor(mRawRenderer, int(mClearColour.x), int(mClearColour.y), int(mClearColour.z), 255);
    SDL_RenderClear(mRawRenderer);
}

void Renderer::Refresh() {
    SDL_RenderPresent(mRawRenderer);
}

Texture* Renderer::RenderScene(Texture* image) {
    Sphere s = Sphere({ 0, 0, 5 }, 0.2);
    Camera camera = Camera(1, 1, { 0, 0, 0 });

    image->Lock();

    for (int y = 0; y < 500; y++) {
        for (int x = 0; x < 500; x++) {
            Vector2f screenPos = { x / 500.0f, y / 500.0f };
            Vector3f viewportPos = camera.GetViewportPos(screenPos);
            Ray ray = Ray(camera.position, viewportPos - camera.position);
            Colour colour = { 0, 255, 0 };
            if (s.Intersect(ray))
                colour = { 255, 0, 0 };
            image->SetColourAt({ (float) x, (float) y }, colour);
        }
    }

    image->Unlock();

    return image;
}

}