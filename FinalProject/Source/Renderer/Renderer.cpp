#include "Renderer.h"

#include <thread>
#include <chrono>
#include <random>
#include <SDL.h>
#include <Window/Window.h>
#include <Maths/Vector2f.h>
#include <Maths/Vector3f.h>
#include <Maths/Sampling.h>
#include <Renderer/RayPayload.h>

#include <nlohmann/json.hpp>

#include "Sphere.h"
#include "Camera.h"

#include <Error.h>

using json = nlohmann::json;

namespace Prototype {

nlohmann::json RenderSettings::ToJSON() {
    json data = json();
    data["resolution"]          = resolution.ToJSON();
    data["maxDepth"]            = maxDepth;
    data["samples"]             = samples;
    data["ambientLight"]        = ambientLight.ToJSON();
    data["checkerboard"]        = checkerboard;
    data["directLightSampling"] = directLightSampling;

    data["mode"] = mode;
    data["fastMode"] = fastMode;

    return data;
}

void RenderSettings::LoadFromJSON(json data) {
    resolution          = Vector2f::LoadFromJSON(data["resolution"]);
    maxDepth            = data["maxDepth"];
    samples             = data["samples"];
    ambientLight        = Vector3f::LoadFromJSON(data["ambientLight"]);
    checkerboard        = data["checkerboard"];
    directLightSampling = data["directLightSampling"];

    mode                = (RenderMode)data["mode"];
    fastMode            = data["fastMode"];
}


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

Colour Renderer::GatherDirectLighting(const Scene& scene, const RayPayload& payload, std::mt19937& rnd) {
    Colour light = Vector3f(0, 0, 0);

    // Accumulate direct lighting from each light source.
    for each (Object* obj in scene.GetLights()) {
        if (obj == payload.object) continue; // Don't accumulate light from self.
        if (!obj->show) continue;
        //Vector3f direction = obj->GetPosition() - payload.point;
        float pdf2 = 0;
        Vector3f direction = obj->Sample(payload.point, pdf2, rnd);
        if (direction == Vector3f()) direction = obj->GetPosition() - payload.point;
        direction.Normalize();
        Ray toLight = Ray(payload.point + payload.normal * 0.01, direction);
        RayPayload toLightPayload;
        scene.ClosestHit(toLight, 0.001, FLT_MAX, toLightPayload);
        if (toLightPayload.object == obj) {
            // Hit the light.
            Vector3f normal = payload.frontFace ? payload.normal : -1 * payload.normal; // Make sure normal is pointing in the right direction.
            float pdf = payload.material->GetPDF(toLight.GetDirection(), payload);

            // Set the contribution to 0 if the angle is more than 90
            if (direction.Dot(normal) <= 0) pdf = 0;

            float distance = (toLightPayload.point - payload.point).Magnitude();

            light += obj->material.Emit() * pdf / pdf2;
        }
    }

    return light;
}

Colour Renderer::TraceRay(Scene& scene, float* depthMap, const Ray& ray, int depth, bool lightNext, const RenderSettings& settings, std::mt19937& rnd) {
    // Don't go on forever!
    if (depth >= settings.maxDepth) {
        return Vector3f();
    }
    depth++;

    RayPayload payload;
    if (scene.ClosestHit(ray, 0.001, FLT_MAX, payload)) {
        //if (payload.object->material.emitted != 0) return payload.material->Emit();
        Ray newRay = Ray(Vector3f(), Vector3f());
        float pdf = 1;
        payload.material->Scatter(ray, newRay, pdf, payload, rnd);
        Colour colour = Vector3f(0, 0, 0);

        if (depth == 1) {
            *depthMap = payload.t;
            //if (payload.object->material.materialType == MaterialType::Glass) *depthMap *= -1;
            if (settings.fastMode) return payload.material->colour / payload.t * 4;
            if (settings.mode == RenderMode::DepthBuffer) return Vector3f(1, 1, 1) / payload.t * 4;
        }

        if (settings.directLightSampling) {
            Colour light = GatherDirectLighting(scene, payload, rnd);

            // If this is the first bounce, add the emitted lighting so that lights appear bright.
            // Of if hit a light and the last object hit should have a highlight.
            // TODO: make sure lights are only sampled once!!
            if (depth == 1 || (lightNext && payload.material->emitted != 0)) light += payload.material->Emit();
            colour += light * payload.material->colour;
        }
        else {
            colour += payload.material->Emit();
        }

        //if (!payload.frontFace) return Vector3f();
        if (settings.mode == RenderMode::Normals) return 0.5 * (payload.normal + Vector3f(1, 1, 1));
        lightNext = false;
        if (payload.material->materialType != MaterialType::Lambertian  && depth == 1) lightNext = true;

        return colour + TraceRay(scene, depthMap, newRay, depth, lightNext, settings, rnd) * payload.material->colour;
    }
    else {
        if (depth == 1) {
            *depthMap = FLT_MAX;
        }

        return settings.ambientLight;
    }
    return { 0, 0, 0 };
}

void Renderer::RenderStrip(Scene scene, Colour** image, float** depthMap, const RenderSettings& settings, int frame, int start, int end) {
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
                    viewportPosRand.Normalize();
                    Ray rayRand = Ray(scene.camera.GetPosition(), viewportPosRand);


                    colour += TraceRay(scene, &depthMap[x][y], rayRand, 0, false, settings, rnd);
                    //if (depthMap[x][y] > 0) colour += Vector3f(1, 1, 1) / depthMap[x][y];
                }
                colour /= settings.samples;

                //if (depthMap[x][y] != 0) colour = Vector3f(1, 1, 1) / depthMap[x][y];

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

Colour** Renderer::RenderScene(Scene scene, Colour** image, float** depthMap, const RenderSettings& settings, int frame) {
    int strips = 10;
    std::vector<std::thread> threads;

    int size = settings.resolution.x / strips;
    for (int i = 0; i < strips; i++) {
        threads.push_back(std::thread([=] { RenderStrip(scene, image, depthMap, settings, frame, i * size, (i + 1) * size); }));
    }

    for (int i = 0; i < threads.size(); i++) {
        threads[i].join();
    }

    //RenderStrip(scene, image, settings, frame, 0, settings.resolution.x);

    return image;
}

}