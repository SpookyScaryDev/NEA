#include "Scene.h"

#include <vector>
#include <Renderer/Object.h>
#include <Renderer/Camera.h>
#include <Renderer/RayPayload.h>

namespace Prototype {

Scene::Scene() {}

void Scene::AddObject(Object* object) {
    mObjects.push_back(object);
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
        if (object->Intersect(ray, min, closestT, tempPayload)) {
            if (tempPayload.t < closestT) {
                closestPayload = tempPayload;
                closestT = tempPayload.t;
            }
        }
    }
    if (closestT == max) return false;
    payload = closestPayload;
    return true;
}


}