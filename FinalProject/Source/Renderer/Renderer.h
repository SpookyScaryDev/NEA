#pragma once

#include <Maths/Vector2f.h>
#include <Maths/Vector3f.h>
#include <Renderer/Texture.h>
#include <Renderer/Scene.h>
#include <Window/Window.h>
#include <random>
#include <nlohmann/json.hpp>

struct SDL_Renderer;

namespace Prototype {

enum class RenderMode {
    RayTraced,
    DepthBuffer,
    Normals
};

struct RenderSettings {
    Vector2f            resolution;
    int                 maxDepth;
    int                 samples;
    Colour              ambientLight;
    bool                checkerboard;
    bool                directLightSampling;
    int                 numThreads;

    RenderMode          mode;
    bool                fastMode;

    nlohmann::json      ToJSON();
    void                LoadFromJSON(nlohmann::json data);
};

class Renderer { 
public:
                            Renderer();

    void                    Init(Window* window);
    void                    Shutdown();

    SDL_Renderer*           GetRawRenderer();

    void                    SetClearColour(const Colour& colour);
    void                    Clear();                              // Clear screen to the clear colour.
    void                    Refresh();                            // Draw to the screen.

    Colour                  TraceRay(Scene& scene, float* depthMap, const Ray& ray, int depth, bool lightNext, const RenderSettings& settings, std::mt19937& rnd);
    Colour**                RenderScene(Scene scene, Colour** image, float** depthMap, const RenderSettings& settings, int frame = 0);

private:
    void                    RenderStrip(Scene scene, Colour** image, float** depthMap, const RenderSettings& settings, int frame, int start, int end);
    Colour                  GatherDirectLighting(const Scene& scene, const RayPayload& payload, std::mt19937& rnd);

    SDL_Renderer*           mRawRenderer;
    Vector3f                mClearColour;
};

}