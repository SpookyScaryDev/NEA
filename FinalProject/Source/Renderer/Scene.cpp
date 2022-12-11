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

Scene::Scene(const char* filePath) : mFilePath(filePath)
{
    mModified = false;
}

Scene Scene::LoadFromFile(const char* filePath) {
    Scene scene(filePath);
    std::ifstream file(filePath);
    json data = json::parse(file);

    std::cout << "Loading " << filePath << std::endl;
    std::cout << "************************************************************" << std::endl;

    scene.SetCamera(Camera::LoadFromJSON(data["camera"]));
    for each (json obj in data["objects"]) {
        std::cout << "Loading object " << obj["name"].get<std::string>() << std::endl;
        Object* object = Object::LoadFromJSON(obj);
        scene.AddObject(obj["name"].get<std::string>().c_str(), object);
    }

    std::cout << "************************************************************" << std::endl;
    file.close();
    return scene;
}

void Scene::SaveToFile(const char* filePath) {
    mModified = false;
    json data;
    data["camera"] = camera.ToJSON();
    for each (Object* object in mObjects) {
        data["objects"].push_back(object->ToJSON());
    }
    std::ofstream file(filePath);
    file << std::setw(4) << data << std::endl;
    file.close();
    mFilePath = filePath;
}

std::string Scene::GetName() const {
    std::string name = mFilePath;
    name = name.substr(name.find_last_of("/\\") + 1); // Remove path.
    name = name.substr(0, name.find_last_of('.')); // Remove extension.
    return name.c_str();
}

std::string Scene::GetPath() const {
    return mFilePath;
}

void Scene::SetModified() {
    mModified = true;
}

bool Scene::IsModified() const {
    return mModified;
}

void Scene::Save() {
    SaveToFile(mFilePath.c_str());
}

void Scene::AddObject(const char* name, Object* object) {
    object->name = name;
    mObjects.push_back(object);
}

void Scene::RemoveObject(Object* object) {
    mObjects.erase(std::remove(mObjects.begin(), mObjects.end(), object));
}

int Scene::GetObjectID(Object* object) const {
    return std::find(mObjects.begin(), mObjects.end(), object) - mObjects.begin();
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

std::vector<Object*> Scene::GetLights() const {
    std::vector<Object*> lights;
    for each (Object* obj in mObjects) {
        if (obj->material.emitted != 0) lights.push_back(obj);
    }
    return lights;
}

bool Scene::ClosestHit(const Ray& ray, float min, float max, RayPayload& payload) const{
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

void Scene::UpdateObjects() {
    camera.Update();
    for each (Object* object in mObjects) {
        object->Update();
    }
}


}