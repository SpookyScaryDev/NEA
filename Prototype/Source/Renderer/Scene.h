#pragma once

#include <vector>
#include <Renderer/Object.h>
#include <Renderer/Camera.h>
#include <Renderer/RayPayload.h>

namespace Prototype {

class Scene {
public:
                           Scene();
    void                   AddObject(Object* object);
    void                   SetCamera(const Camera& cam);
    std::vector<Object*>&  GetObjects();
    bool                   ClosestHit(const Ray& ray, float min, float max, RayPayload& payload);

    Camera                 camera;
private:
    std::vector<Object*>   mObjects;
};

}