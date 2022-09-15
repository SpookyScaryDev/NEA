#include "Scene.h"

#include <vector>
#include <Renderer/Object.h>
#include <Renderer/Camera.h>
#include <Renderer/RayPayload.h>

namespace Prototype {

Scene::Scene() {}

void Scene::AddObject(const char* name, Object* object) {
    object->name = name;
    object->id = GetObjectCount();
    mObjects.push_back(object);
}

int Scene::GetObjectCount() const {
    return mObjects.size();
}

void Scene::SetCamera(const Camera& cam) {
    camera = cam;
}

std::vector<Object*>& Scene::GetObjects() {
    return mObjects;
}

bool Scene::ClosestHit(const Ray& ray, float min, float max, RayPayload& payload) {
    RayPayload closestPayload;
    RayPayload tempPayload;
    float closestT = max;
    for each (Object* object in mObjects) {
        if (object->show) {
            if (object->Intersect(ray, min, closestT, tempPayload)) {
                if (tempPayload.t < closestT) {
                    closestPayload = tempPayload;
                    closestT = tempPayload.t;
                }
            }
        }
    }
    if (closestT == max) return false;
    payload = closestPayload;
    return true;
}


}