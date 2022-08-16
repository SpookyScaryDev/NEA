#include "Texture.h"

#include <SDL.h>
#include <Renderer/Renderer.h>
#include <Application/Application.h>
#include <Maths/Vector2f.h>

namespace Prototype {

Texture::Texture(int width, int height) {
    //TODO: Error check this!
    mTextureData = SDL_CreateTexture(Application::GetApp()->GetRenderer()->GetRawRenderer(), SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_STREAMING, width, height);
    Uint32* format = (Uint32*) &mPixelFormat;
    SDL_QueryTexture(mTextureData, format, nullptr, &mWidth, &mHeight);
    mPixelFormat= SDL_AllocFormat((Uint32)mPixelFormat);
}

Texture::Texture(const std::string& filePath):
    mFilePath(filePath) {

    //JEM_CORE_MESSAGE("Loading Texture \"", mFilePath, "\"");
    SDL_Surface* surface = SDL_LoadBMP(filePath.c_str());
    mTextureData = SDL_CreateTextureFromSurface(Application::GetApp()->GetRenderer()->GetRawRenderer(), surface);
    //JEM_CORE_ASSERT(mTextureData, "Couldn't load texture \"" + mFilePath + "\"");
    SDL_FreeSurface(surface);

    Uint32* format = (Uint32*)&mPixelFormat;
    SDL_QueryTexture(mTextureData, format, nullptr, &mWidth, &mHeight);
    mPixelFormat = SDL_AllocFormat((Uint32)mPixelFormat);
}

Texture::Texture(SDL_Texture* texture) :
    mFilePath("") {

    mTextureData = texture;
    mPixelData = nullptr;

    Uint32* format = (Uint32*)&mPixelFormat;
    SDL_QueryTexture(mTextureData, format, nullptr, &mWidth, &mHeight);
    mPixelFormat = SDL_AllocFormat((Uint32)mPixelFormat);
}

// ==================
// Prototype::Texture::~Texture
// ==================
Texture::~Texture() {
    if (mTextureData != nullptr) {
        SDL_DestroyTexture(mTextureData);
        mTextureData = nullptr;
    }
}

// ==================
// Prototype::Texture::GetRawTexture
// ==================
SDL_Texture* Texture::GetRawTexture() const {
    return mTextureData;
}

// ==================
// Prototype::Texture::GetWidth
// ==================
int Texture::GetWidth() const {
    return mWidth;
}

// ==================
// Prototype::Texture::GetHeight
// ==================
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

Vector3f Texture::GetColourAt(const Point2f& position) {
    Uint32* pixels = (Uint32*)mPixelData;
    Uint8 r;
    Uint8 g;
    Uint8 b;
    SDL_GetRGB(pixels[(int)(position.y * (mPitch / sizeof(unsigned int)) + position.x)], mPixelFormat, &r , &g, &b);

    return { (float)r, (float)g, (float)b };
}

void Texture::SetColourAt(const Point2f& position, const Colour& colour) {
    Uint32 pixel = SDL_MapRGB(mPixelFormat, colour.x, colour.y, colour.z);
    Uint32* pixels = (Uint32*)mPixelData;
    pixels[(int)(position.y * (mPitch / sizeof(unsigned int)) + position.x)] = pixel;
}

}