#pragma once

#include <string>
#include <vector>
#include <Renderer/Object.h>
#include <Renderer/Camera.h>
#include <Renderer/RayPayload.h>

#include <nlohmann/json.hpp>

namespace rtos {

// A scene owns a set of objects and a camera.
class Scene {
public:
                           Scene();
                           Scene(const char* name);

    static Scene           LoadFromFile(const char* filePath);
    void                   Save();
    void                   SaveToFile(const char* filePath);

    std::string            GetName() const;
    std::string            GetPath() const;
    void                   SetModified();
    bool                   IsModified() const;

    void                   AddObject(const char* name, Object* object);
    void                   RemoveObject(Object* object);
    int                    GetObjectID(Object* object) const;
    int                    GetObjectCount() const;
    void                   SetCamera(const Camera& cam);
    std::vector<Object*>&  GetObjects();
    std::vector<Object*>   GetLights() const;
    bool                   ClosestHit(const Ray& ray, float min, float max, RayPayload& payload) const; // Returns true if something is hit. Details are stored in the payload.

    void                   UpdateObjects();

    Camera                 camera;
private:
    bool                   mModified;
    std::vector<Object*>   mObjects;
    std::string            mFilePath;
};

}