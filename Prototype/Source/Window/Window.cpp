#include "Window.h"

#include <SDL.h>

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
        SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE
    );

    if (mRawSDLWindow == nullptr) {
        // TODO! 
        // The window could not be created.
        //JEM_CORE_ERROR("InitWindow: Failed to create window - ", SDL_GetError());
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

}
