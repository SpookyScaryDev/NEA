#include "Scene.h"

#include <vector>
#include <Renderer/Object.h>
#include <Renderer/Sphere.h>
#include <Renderer/Mesh.h>
#include <Renderer/Camera.h>
#include <Renderer/RayPayload.h>

#include <fstream>
#include <nlohmann/json.hpp>

#include <iostream>
#include <string>

using json = nlohmann::json;

namespace Prototype {

Scene::Scene() {}

Scene Scene::LoadFromFile(const char* filePath) {
    Scene scene;
    std::ifstream file(filePath);
    json data = json::parse(file);

    scene.SetCamera(Camera::LoadFromJSON(data["camera"]));
    for each (json obj in data["objects"]) {
        std::cout << obj["name"] << std::endl;
        Object* object = Object::LoadFromJSON(obj);
        scene.AddObject(obj["name"].get<std::string>().c_str(), object);
    }

    file.close();
    return scene;
}

void Scene::SaveToFile(const char* filePath) {
    json data;
    data["camera"] = camera.ToJSON();
    for each (Object* object in mObjects) {
        data["objects"].push_back(object->ToJSON());
    }
    std::ofstream file(filePath);
    file << std::setw(4) << data << std::endl;
    file.close();
}

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