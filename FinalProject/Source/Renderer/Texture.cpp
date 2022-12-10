#include "Texture.h"

#include <SDL.h>
#include <Renderer/Renderer.h>
#include <Application/Application.h>
#include <Maths/Vector2f.h>

#include <Error.h>

namespace Prototype {

Texture::Texture(int width, int height) {
    mTextureData = SDL_CreateTexture(Application::GetApp()->GetRenderer()->GetRawRenderer(), SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_STREAMING, width, height);
    if (mTextureData == nullptr) {
        ERROR(std::string("Failed to create texture!") + SDL_GetError());
    }
    Uint32* format = (Uint32*) &mPixelFormat;
    SDL_QueryTexture(mTextureData, format, nullptr, &mWidth, &mHeight);
    mPixelFormat= SDL_AllocFormat((Uint32)mPixelFormat);
}

Texture::Texture(SDL_Texture* texture) {
    mTextureData = texture;
    mPixelData = nullptr;

    Uint32* format = (Uint32*)&mPixelFormat;
    SDL_QueryTexture(mTextureData, format, nullptr, &mWidth, &mHeight);
    mPixelFormat = SDL_AllocFormat((Uint32)mPixelFormat);
}

Texture::~Texture() {
    if (mTextureData != nullptr) {
        SDL_DestroyTexture(mTextureData);
        mTextureData = nullptr;
    }
}

SDL_Texture* Texture::GetRawTexture() const {
    return mTextureData;
}

int Texture::GetWidth() const {
    return mWidth;
}

int Texture::GetHeight() const {
    return mHeight;
}

void Texture::Unlock() {
    SDL_UnlockTexture(mTextureData);
    mPixelData = nullptr;
}

void Texture::Lock() {
    SDL_LockTexture(mTextureData, NULL, &mPixelData, &mPitch);
}

Vector3f Texture::GetColourAt(const Vector2f& position) {
    Uint32* pixels = (Uint32*)mPixelData;
    Uint8 r;
    Uint8 g;
    Uint8 b;
    SDL_GetRGB(pixels[(int)(position.y * (mPitch / sizeof(unsigned int)) + position.x)], mPixelFormat, &r , &g, &b);

    return { (float)r, (float)g, (float)b };
}

void Texture::SetColourAt(const Vector2f& position, const Colour& colour) {
    Uint32 pixel = SDL_MapRGB(mPixelFormat, colour.x, colour.y, colour.z);
    Uint32* pixels = (Uint32*)mPixelData;
    pixels[(int)(position.y * (mPitch / sizeof(unsigned int)) + position.x)] = pixel;
}

void Texture::SaveToFile(const char* path) {
    Lock();
    SDL_Surface* surface = SDL_CreateRGBSurfaceFrom(mPixelData, mWidth, mHeight, mPixelFormat->BitsPerPixel, mPitch, mPixelFormat->Rmask, mPixelFormat->Gmask, mPixelFormat->Bmask, mPixelFormat->Amask);
    Unlock();
    SDL_SaveBMP(surface, path);
}

}