#pragma once

#include <string>
#include <Maths/Vector2f.h>
#include <Maths/Vector3f.h>

struct SDL_Texture;
struct SDL_PixelFormat;

namespace Prototype {

class Texture {
public:
                       Texture(int width, int height);
                       Texture(SDL_Texture* texture);
                       ~Texture();

    SDL_Texture*       GetRawTexture() const;
    int                GetWidth() const;
    int                GetHeight() const;

    void               Unlock(); // Allow editing.
    void               Lock();   // Apply changes.

    Vector3f           GetColourAt(const Vector2f& position);
    void               SetColourAt(const Vector2f& position, const Colour& colour);

private:
    SDL_Texture*       mTextureData;
    void*              mPixelData;
    SDL_PixelFormat*   mPixelFormat;
    int                mPitch;
    int                mWidth;
    int                mHeight;
};

}