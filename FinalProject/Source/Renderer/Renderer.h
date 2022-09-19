#pragma once

#include <Maths/Vector2f.h>
#include <Maths/Vector3f.h>
#include <Renderer/Texture.h>
#include <Renderer/Scene.h>
#include <Window/Window.h>
#include <random>

struct SDL_Renderer;

namespace Prototype {

struct RenderSettings {
    Vector2f            resolution;
    int                 maxDepth;
    int                 samples;
    Colour              ambientLight;
    bool                checkerboard;
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

    Colour                  TraceRay(Scene& scene, const Ray& ray, int depth, const RenderSettings& settings, std::mt19937& rnd);
    Colour**                RenderScene(Scene scene, Colour** image, const RenderSettings& settings, int frame = 0);

private:
    void                    RenderStrip(Scene scene, Colour** image, const RenderSettings& settings, int frame, int start, int end);

    SDL_Renderer*           mRawRenderer;
    Vector3f                mClearColour;
};

}