#pragma once

#include <vector>
#include <Renderer/Object.h>
#include <Renderer/Camera.h>
#include <Renderer/RayPayload.h>

#include <nlohmann/json.hpp>

namespace Prototype {

// A scene owns a set of objects and a camera.
class Scene {
public:
                           Scene();

    static Scene           LoadFromFile(const char* filePath);
    void                   SaveToFile(const char* filePath);

    void                   AddObject(const char* name, Object* object);
    int                    GetObjectCount() const;
    void                   SetCamera(const Camera& cam);
    std::vector<Object*>&  GetObjects();
    bool                   ClosestHit(const Ray& ray, float min, float max, RayPayload& payload); // Returns true if something is hit. Details are stored in the payload.

    Camera                 camera;
private:
    std::vector<Object*>   mObjects;
};

}