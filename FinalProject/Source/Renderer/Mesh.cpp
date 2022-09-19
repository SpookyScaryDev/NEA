#include "Mesh.h"

#include <vector>
#include <Renderer/Object.h>
#include <Renderer/Triangle.h>
#include <Renderer/Scene.h>
#include <fstream>
#include <sstream>
#include <iostream>
#include <string>

namespace Prototype {

Mesh::Mesh(Vector3f position, const char* filePath, Material material) :
	Object(position, material)
{
	LoadFromFile(filePath);
}

void Mesh::LoadFromFile(const char* filePath) {
	std::vector<Vector3f> verticies;

	std::ifstream file(filePath);

	std::string line;
	std::vector<std::string> splitLine;
	while (std::getline(file, line)) {

		splitLine = Split(line, ' ');

		if (splitLine[0] == "v") {
			mMin.x = fmin(mMin.x, std::stof(splitLine[1]));
			mMin.y = fmin(mMin.y, std::stof(splitLine[2]));
			mMin.z = fmin(mMin.z, std::stof(splitLine[3]));

			mMax.x = fmax(mMax.x, std::stof(splitLine[1]));
			mMax.y = fmax(mMax.y, std::stof(splitLine[2]));
			mMax.z = fmax(mMax.z, std::stof(splitLine[3]));

			verticies.push_back({ std::stof(splitLine[1]), std::stof(splitLine[2]), std::stof(splitLine[3]) });
		}

		if (splitLine[0] == "f") {
			Vector3f faceVerticies[3];
			int v1 = std::stof(Split(splitLine[1], '/')[0]);
			int v2 = std::stof(Split(splitLine[2], '/')[0]);
			int v3 = std::stof(Split(splitLine[3], '/')[0]);
			faceVerticies[0] = verticies[std::stof(Split(splitLine[1], '/')[0]) - 1];
			faceVerticies[1] = verticies[std::stof(Split(splitLine[2], '/')[0]) - 1];
			faceVerticies[2] = verticies[std::stof(Split(splitLine[3], '/')[0]) - 1];
			Triangle* face = new Triangle(Vector3f(), faceVerticies, material);
			mFaces.push_back(face);

			if (splitLine.size() == 5) {
				faceVerticies[0] = verticies[std::stof(Split(splitLine[1], '/')[0]) - 1];
				faceVerticies[1] = verticies[std::stof(Split(splitLine[3], '/')[0]) - 1];
				faceVerticies[2] = verticies[std::stof(Split(splitLine[4], '/')[0]) - 1];
				face = new Triangle(Vector3f(), faceVerticies, material);
				mFaces.push_back(face);
			}
		}
	}
}

bool Mesh::Intersect(const Ray& ray, float min, float max, RayPayload& payload) {
	// TODO: OOF!
	Vector3f transformedMin = mMin * scale + position;
	Vector3f transformedMax = mMax * scale + position;

	float tx1 = (transformedMin.x - ray.GetOrigin().x) / ray.GetDirection().x;
	float tx2 = (transformedMax.x - ray.GetOrigin().x) / ray.GetDirection().x;

	float tmin = fmin(tx1, tx2);
	float tmax = fmax(tx1, tx2);

	float ty1 = (transformedMin.y - ray.GetOrigin().y) / ray.GetDirection().y;
	float ty2 = (transformedMax.y - ray.GetOrigin().y) / ray.GetDirection().y;

	tmin = fmax(tmin, fmin(ty1, ty2));
	tmax = fmin(tmax, fmax(ty1, ty2));

	float tz1 = (transformedMin.z - ray.GetOrigin().z) / ray.GetDirection().z;
	float tz2 = (transformedMax.z - ray.GetOrigin().z) / ray.GetDirection().z;

	tmin = fmax(tmin, fmin(tz1, tz2));
	tmax = fmin(tmax, fmax(tz1, tz2));

	if (tmax >= tmin) {
		RayPayload closestPayload;
		RayPayload tempPayload;
		float closestT = max;
		for each (Triangle * triangle in mFaces) {
			Object* object = (Object*)triangle;
			object->position = position;
			object->scale = scale;
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
		payload.object = this;
		payload.material = &material;
		return true;
	}

	return false;
}

std::vector<std::string> Mesh::Split(const std::string& string, char character) {
	std::vector<std::string> splitString;
	std::istringstream stream(string);
	std::string word;
	while (std::getline(stream, word, character)) {
		splitString.push_back(word);
	}

	return splitString;
}

}