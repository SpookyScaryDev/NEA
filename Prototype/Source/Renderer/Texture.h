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
                       Texture(const std::string& filePath);
                       Texture(SDL_Texture* texture);
                       ~Texture();

    SDL_Texture*       GetRawTexture() const;
    int                GetWidth() const;
    int                GetHeight() const;

    void               Unlock();
    void               Lock();

    Vector3f           GetColourAt(const Point2f& position);
    void               SetColourAt(const Point2f& position, const Colour& colour);

private:
    std::string        mFilePath;
    SDL_Texture*       mTextureData;
    void*              mPixelData;
    SDL_PixelFormat*   mPixelFormat;
    int                mPitch;
    int                mWidth;
    int                mHeight;
};

}