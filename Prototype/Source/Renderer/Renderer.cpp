#include "Renderer.h"

#include <SDL.h>
#include <Window/Window.h>
#include <Maths/Vector2f.h>
#include <Maths/Vector3f.h>
#include <Renderer/RayPayload.h>

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

Colour Renderer::TraceRay(Scene& scene, const Ray& ray, int depth, const RenderSettings& settings) {
    if (depth >= settings.maxDepth) {
        //for each (Object * object in scene.GetObjects())
        //{
        //    if (object->material.emitted.x != 0 && object->material.emitted.y != 0 && object->material.emitted.z != 0) {
        //        Vector3f pos = object->position;
        //        RayPayload payload;
        //        Vector3f dir = object->position - ray.GetOrigin();
        //        dir.Normalize();
        //        Ray toLight = Ray(ray.GetOrigin(), dir);
        //        if (scene.ClosestHit(toLight, -0.1, FLT_MAX, payload)) {
        //            float potDist = (payload.point - ray.GetOrigin()).Magnitude();
        //            if (pos.x == payload.object->position.x && pos.y == payload.object->position.y && pos.z == payload.object->position.z) {
        //                //return { 100, 100, 0 };
        //                Colour colour = object->material.emitted;
        //                colour.x = fmin(colour.x, 1);
        //                colour.y = fmin(colour.y, 1);
        //                colour.z = fmin(colour.z, 1);
        //                return object->material.emitted * (1.0 / pow(payload.t, 2.0));
        //            }
        //        }
        //    }
        //}
        return Vector3f();
        //return {0.2, 0.2, 0.2};
    }
    depth++;

    Colour light;


    RayPayload payload;
    if (scene.ClosestHit(ray, 0.01, FLT_MAX, payload)) {
        //return (payload.normal + Vector3f(1, 1, 1)) * 255 * 0.5;
        Ray newRay = Ray(Vector3f(), Vector3f());
        payload.material->Scatter(ray, newRay, payload);
        return TraceRay(scene, newRay, depth, settings) * payload.material->colour + payload.material->emitted + light;
    }
    else {
        return settings.ambientLight;
    }
    return { 0, 0, 0 };
}

Colour** Renderer::RenderScene(Scene scene, Colour** image, const RenderSettings& settings, int frame) {

    for (int y = 0; y < settings.resolution.y; y++) {
        for (int x = 0; x < settings.resolution.x; x++) {
            Colour colour;
            for (size_t i = 0; i < settings.samples; i++) {
                Vector2f screenPos = { x / settings.resolution.x, y / settings.resolution.y };
                screenPos.x += (rand() / (RAND_MAX + 1.0)) / (settings.resolution.x + 1);
                screenPos.y += (rand() / (RAND_MAX + 1.0)) / (settings.resolution.y + 1);
                Vector3f viewportPos = scene.camera.GetViewportPos(screenPos);
                Ray ray = Ray(scene.camera.position, viewportPos);
                colour+= TraceRay(scene, ray, 0, settings);
            }
            colour /= settings.samples;

            Colour oldColour = image[x][y];
            Colour finalColour = ((oldColour * (frame-1)) + colour) / (frame);
            //Colour finalColour = ((oldColour * (9)) + colour) / (10);

            finalColour.x = fmin(finalColour.x, 1);
            finalColour.y = fmin(finalColour.y, 1);
            finalColour.z = fmin(finalColour.z, 1);

            image[x][y] = finalColour;
        }
    }

    return image;
}

}