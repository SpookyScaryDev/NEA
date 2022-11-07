#include "Renderer.h"

#include <thread>
#include <chrono>
#include <random>
#include <SDL.h>
#include <Window/Window.h>
#include <Maths/Vector2f.h>
#include <Maths/Vector3f.h>
#include <Renderer/RayPayload.h>

#include "Sphere.h"
#include "Camera.h"

#include <Error.h>

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
        ERROR(std::string("Failed to create renderer - ") + SDL_GetError());
    }

    SDL_SetRenderDrawBlendMode(mRawRenderer, SDL_BLENDMODE_BLEND);

    SDL_RendererInfo rendererInfo;
    SDL_GetRendererInfo(mRawRenderer, &rendererInfo);
    printf("Renderer Info: ");
    printf("API: %s\n", rendererInfo.name);

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

Colour Renderer::TraceRay(Scene& scene, const Ray& ray, int depth, const RenderSettings& settings, std::mt19937& rnd) {
    // Don't go on forever!
    if (depth >= settings.maxDepth) {
        return Vector3f();
    }
    depth++;

    RayPayload payload;
    if (scene.ClosestHit(ray, 0.001, FLT_MAX, payload)) {
        Ray newRay = Ray(Vector3f(), Vector3f());
        float pdf = 1;
        payload.material->Scatter(ray, newRay, pdf, payload, rnd);
        return TraceRay(scene, newRay, depth, settings, rnd) * payload.material->colour + payload.material->Emit();
    }
    else {
        return settings.ambientLight;
    }
    return { 0, 0, 0 };
}

void Renderer::RenderStrip(Scene scene, Colour** image, const RenderSettings& settings, int frame, int start, int end) {
    //srand(static_cast<int>(time(0)));
    std::random_device r;
    std::seed_seq seed{ r(), r(), r(), r(), r(), r(), r(), r() };
    std::mt19937 rnd(seed);
    std::uniform_real_distribution<float> dist(0.0, 1.0);

    for (int y = 0; y < settings.resolution.y; y++) {
        for (int x = start; x < end; x++) {
            if (!settings.checkerboard || ((x % 2 == 0 && y % 2 == 0) || (x % 2 != 0 && y % 2 != 0))) {
                Vector2f screenPos = { x / settings.resolution.x, y / settings.resolution.y };
                Vector3f viewportPos = scene.camera.GetViewportPos(screenPos);

                Colour colour;
                for (size_t i = 0; i < settings.samples; i++) {
                    // Slightly randomize position (anti-aliasing).
                    screenPos.x = (x + (dist(rnd) - 0.5)) / (settings.resolution.x);
                    screenPos.y = (y + (dist(rnd) - 0.5)) / (settings.resolution.y);

                    Vector3f viewportPosRand = scene.camera.GetViewportPos(screenPos);
                    Ray rayRand = Ray(scene.camera.position, viewportPosRand);


                    colour += TraceRay(scene, rayRand, 0, settings, rnd);
                }
                colour /= settings.samples;

                // Average colour over time.
                Colour oldColour = image[x][y];
                Colour finalColour = ((oldColour * (frame - 1)) + colour) / (frame);

                // Clamp colour so it doesn't overflow.
                finalColour.x = fmin(finalColour.x, 1);
                finalColour.y = fmin(finalColour.y, 1);
                finalColour.z = fmin(finalColour.z, 1);

                image[x][y] = finalColour;
            }
        }
    }
}

Colour** Renderer::RenderScene(Scene scene, Colour** image, const RenderSettings& settings, int frame) {
    int strips = 10;
    std::vector<std::thread> threads;

    int size = settings.resolution.x / strips;
    for (int i = 0; i < strips; i++) {
        threads.push_back(std::thread([=] { RenderStrip(scene, image, settings, frame, i * size, (i + 1) * size); }));
    }

    for (int i = 0; i < threads.size(); i++) {
        threads[i].join();
    }

    //RenderStrip(scene, image, settings, frame, 0, settings.resolution.x);

    return image;
}

}