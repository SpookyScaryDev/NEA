#include "Mesh.h"

#include <vector>
#include <Maths/Sampling.h>
#include <Renderer/Object.h>
#include <Renderer/Triangle.h>
#include <Renderer/Scene.h>
#include <fstream>
#include <sstream>
#include <iostream>
#include <string>

#include <nlohmann/json.hpp>

using json = nlohmann::json;

namespace rtos {

Mesh::Mesh(Vector3f position, const char* filePath, Material material) :
	Object(position, material, ObjectType::Mesh)
{
	mFilePath = filePath;
	LoadFromFile(filePath);
}

nlohmann::json Mesh::ToJSON() {
	json data = Object::ToJSON();
	data["type"] = "mesh";
	data["path"] = mFilePath;

	return data;
}

void Mesh::LoadFromFile(const char* filePath) {
	std::vector<Vector3f> verticies;

	std::ifstream file(filePath);

	std::string line;
	std::vector<std::string> splitLine;
	while (std::getline(file, line)) {
		if (line == "") continue;

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

void Mesh::Update() {
	if (mDirty) {
		// Apply transformations to mesh
		for each (Triangle * triangle in mFaces) {
			Object* object = (Object*)triangle;

			object->SetPosition(mPosition);
			object->SetRotation(mRotation);
			object->SetScale(mScale);

			object->Update();
		}

		// TODO: check!!
		Vector3f nonRotatedTransformedMin = mMin * mScale;
		Vector3f nonRotatedTransformedMax = mMax * mScale;

		mTransformedMin = Vector3f();
		mTransformedMax = Vector3f();

		Matrix4x4f m = Matrix4x4f::Rotate(mRotation);
		for (int i = 0; i < 3; i++) {
			for (int j = 0; j < 3; j++) {
				float a = m[i][j] * nonRotatedTransformedMin[j];
				float b = m[i][j] * nonRotatedTransformedMax[j];
				mTransformedMin[i] += a < b ? a : b;
				mTransformedMax[i] += a < b ? b : a;
			}
		}
		////////

		mTransformedMin += mPosition;
		mTransformedMax += mPosition;

		mCenter.x = (mTransformedMin.x + mTransformedMax.x) / 2;
		mCenter.y = (mTransformedMin.y + mTransformedMax.y) / 2;
		mCenter.z = (mTransformedMin.z + mTransformedMax.z) / 2;

		mRadius = (mTransformedMax - mTransformedMin).Magnitude() / 2;

		mDirty = false;
	}
}

bool Mesh::Intersect(const Ray& ray, float min, float max, RayPayload& payload) {
	// TODO: check!!
	float tx1 = (mTransformedMin.x - ray.GetOrigin().x) / ray.GetDirection().x;
	float tx2 = (mTransformedMax.x - ray.GetOrigin().x) / ray.GetDirection().x;

	float tmin = fmin(tx1, tx2);
	float tmax = fmax(tx1, tx2);

	float ty1 = (mTransformedMin.y - ray.GetOrigin().y) / ray.GetDirection().y;
	float ty2 = (mTransformedMax.y - ray.GetOrigin().y) / ray.GetDirection().y;

	tmin = fmax(tmin, fmin(ty1, ty2));
	tmax = fmin(tmax, fmax(ty1, ty2));

	float tz1 = (mTransformedMin.z - ray.GetOrigin().z) / ray.GetDirection().z;
	float tz2 = (mTransformedMax.z - ray.GetOrigin().z) / ray.GetDirection().z;

	tmin = fmax(tmin, fmin(tz1, tz2));
	tmax = fmin(tmax, fmax(tz1, tz2));
	////////

	if (tmax >= tmin) {
		RayPayload closestPayload;
		RayPayload tempPayload;
		float closestT = max;
		for each (Triangle * triangle in mFaces) {
			Object* object = (Object*)triangle;
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

Vector3f Mesh::Sample(const Vector3f& point, float& pdf, std::mt19937& rnd) {
	return Sampling::SamplePointInCone(point, mCenter, mRadius, pdf, rnd);
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