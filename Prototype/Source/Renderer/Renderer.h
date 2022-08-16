#pragma once

#include <Maths/Vector2f.h>
#include <Maths/Vector3f.h>
#include <Renderer/Texture.h>
#include <Window/Window.h>

struct SDL_Renderer;

namespace Prototype {

class Renderer { 
public:
                            Renderer();

    void                    Init(Window* window);
    void                    Shutdown();

    SDL_Renderer*           GetRawRenderer();

    void                    SetClearColour(const Colour& colour);
    void                    Clear();                              // Clear screen to the clear colour.
    void                    Refresh();

    Texture*                RenderScene(Texture* image);

private:
    SDL_Renderer*           mRawRenderer;
    Vector3f                mClearColour;
};

}