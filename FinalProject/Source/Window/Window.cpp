#include "Window.h"

#include <SDL.h>

#include <Error.h>

namespace Prototype {

Window::Window() {
    mRawSDLWindow = nullptr;
    mWidth = mHeight = 0;
}

void Window::Init(const char* name, int width, int height) {
    mWidth = width;
    mHeight = height;
    mRawSDLWindow = SDL_CreateWindow(name,
        SDL_WINDOWPOS_UNDEFINED,
        SDL_WINDOWPOS_UNDEFINED,
        mWidth,
        mHeight,
        SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE | SDL_WINDOW_MAXIMIZED
    );

    if (mRawSDLWindow == nullptr) {
        ERROR(std::string("Failed to create window - ") + SDL_GetError());
    }
}

void Window::Destroy() {
    SDL_DestroyWindow(mRawSDLWindow);
}

int Window::GetWidth() {
    return mWidth;
}

int Window::GetHeight() {
    return mHeight;
}

SDL_Window* Window::GetRawWindow() {
    return mRawSDLWindow;
}

void Window::SetTitle(const char* name) {
    SDL_SetWindowTitle(mRawSDLWindow, name);
}

}
