#include "ConvergingLens.h"


#include <Maths/Sampling.h>
#include <Renderer/Material.h>

#include <nlohmann/json.hpp>

using json = nlohmann::json;

namespace Prototype {

	ConvergingLens::ConvergingLens(Vector3f position, float width, Material material) :
		Object(position, material, ObjectType::ConvergingLens),
		mWidth(width)
	{
		mSphere1 = (Object*) new Sphere(position + Vector3f(0, 0, 1 - mWidth), 1, material);
		mSphere2 = (Object*) new Sphere(position - Vector3f(0, 0, 1 - mWidth), 1, material);
	}

	nlohmann::json ConvergingLens::ToJSON() {
		json data = Object::ToJSON();
		data["type"] = "convergingLens";
		data["width"] = mWidth;

		return data;
	}

	void ConvergingLens::Update() {
		if (mDirty) {
			Matrix4x4f rotation = Matrix4x4f::Rotate(mRotation);
			Vector3f posOffset = Vector3f(0, 0, 1 - mWidth);
			posOffset = rotation * posOffset;
			posOffset = posOffset * mScale.x;
			mSphere1->SetPosition(mPosition + posOffset);
			mSphere2->SetPosition(mPosition - posOffset);
			mSphere1->SetScale(mScale);
			mSphere2->SetScale(mScale);

			mSphere1->Update();
			mSphere2->Update();

			mDirty = false;
		}
	}

	bool ConvergingLens::Intersect(const Ray& ray, float min, float max, RayPayload& payload) {
		RayPayload payload1;
		RayPayload payload2;
		bool hit1 = mSphere1->Intersect(ray, 0.000001, FLT_MAX, payload1);
		bool hit2 = mSphere2->Intersect(ray, 0.000001, FLT_MAX, payload2);
		if (hit1 && hit2) {
			if (payload1.frontFace && payload2.frontFace) {
				if (payload1.t < payload2.t) {
					if (payload1.t2 < payload2.t) return false;
					payload = payload2;
				}
				else {
					if (payload2.t2 < payload1.t) return false;
					payload = payload1;
				}
			}
			else if (!payload1.frontFace && !payload2.frontFace) payload = payload1.t < payload2.t ? payload1 : payload2;
			else {
				if (payload1.frontFace && payload1.t < payload2.t) payload = payload1;
				else if (payload2.frontFace && payload2.t < payload1.t) payload = payload2;
				else return false;
			}
			if (payload.t < min || payload.t > max) return false;
			payload.object = this;
			payload.material = &material;
			return true;
		}
		return false;
	}

	float ConvergingLens::GetWidth() const {
		return mWidth;
	}

	void ConvergingLens::SetWidth(float width) {
		mWidth = width;
		mDirty = true;
	}

	Vector3f ConvergingLens::Sample(const Vector3f& point, float& pdf, std::mt19937& rnd) {
		return Sampling::SamplePointInCone(point, mPosition, mScale.x, pdf, rnd);
		return Vector3f();
	}

}
