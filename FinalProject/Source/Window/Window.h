#pragma once

struct SDL_Window;

namespace rtos {

class Window {
public:
                        Window();
    void                Init(const char* name, int width, int height);
    void                Destroy();

    int                 GetWidth();
    int                 GetHeight();

    SDL_Window*         GetRawWindow();

    void                SetTitle(const char* name);

private:
    SDL_Window*         mRawSDLWindow;
    int                 mWidth;
    int                 mHeight;
};

}
