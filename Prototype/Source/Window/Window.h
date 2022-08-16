#pragma once

struct SDL_Window;

namespace Prototype {

class Window {
public:
                        Window();
    void                Init(const char* name, int width, int height);
    void                Destroy();

    int                 GetWidth();
    int                 GetHeight();

    SDL_Window*         GetRawWindow();

private:
    SDL_Window*         mRawSDLWindow;
    int                 mWidth;
    int                 mHeight;
};

}
