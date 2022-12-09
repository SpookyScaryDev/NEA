#include "DivergingLens.h"


#include <Maths/Sampling.h>
#include <Renderer/Material.h>

#include <nlohmann/json.hpp>

using json = nlohmann::json;

namespace Prototype {

	DivergingLens::DivergingLens(Vector3f position, float width, float curvature, Material material) :
		Object(position, material)
	{
		mWidth = 0.5;
		mCurvature = 5;

		mCenter =  (Object*) new Sphere(position, 1, material);
		mSphere1 = (Object*) new Sphere(position + Vector3f(0, 0, mCurvature + mWidth), mCurvature, material);
		mSphere2 = (Object*) new Sphere(position - Vector3f(0, 0, mCurvature + mWidth), mCurvature, material);
	}

	nlohmann::json DivergingLens::ToJSON() {
		json data = Object::ToJSON();
		data["type"] = "divergingLens";
		data["width"] = mWidth;
		data["curvature"] = mCurvature;

		return data;
	}

	void DivergingLens::Update() {
		if (mDirty) {
			Matrix4x4f rotation = Matrix4x4f::Rotate(mRotation);
			mCenter->SetPosition(mPosition);
			mCenter->SetScale(mScale);
			Vector3f outerPos = Vector3f(mCurvature + mWidth, 0, 0);
			outerPos = rotation * outerPos;
			outerPos = outerPos * mScale;
			mSphere1->SetPosition(mPosition + outerPos);
			mSphere1->SetScale(mScale * mCurvature);
			mSphere2->SetPosition(mPosition - outerPos);
			mSphere2->SetScale(mScale * mCurvature);

			mCenter->Update();
			mSphere1->Update();
			mSphere2->Update();

			mDirty = false;
		}
	}

	bool DivergingLens::Intersect(const Ray& ray, float min, float max, RayPayload& payload) {
		RayPayload payloadCenter;
		RayPayload payload1;
		RayPayload payload2;
		RayPayload closerHitSphere;
		bool hitCenter = mCenter->Intersect(ray, min, max, payloadCenter);
		bool hit1 = mSphere1->Intersect(ray, min, max, payload1);
		bool hit2 = mSphere2->Intersect(ray, min, max, payload2);

		payload1.object = mSphere1;
		payload2.object = mSphere2;

		if (hitCenter) {
			if (hit1 && hit2) {
				closerHitSphere = payload1.t < payload2.t ? payload1 : payload2;
			}
			else if (hit1 || hit2) {
				closerHitSphere = hit1 ? payload1 : payload2;
			}
			else {
				// Just hit center sphere.
				payload = payloadCenter;
				payload.material = &material;
				payload.object = this;
				return true;
			}

			if ((closerHitSphere.frontFace && (payloadCenter.t < closerHitSphere.t || payloadCenter.t > closerHitSphere.t2 )) ||
				(!closerHitSphere.frontFace && payloadCenter.t > closerHitSphere.t)) {
				// Came from completely outside and hit middle.
				payload = payloadCenter;
				payload.material = &material;
				payload.object = this;
				return true;
			}

			if (closerHitSphere.frontFace) {
				if (closerHitSphere.t < payloadCenter.t && closerHitSphere.t2 < payloadCenter.t2) {
					// Came from completely outside and hit lens.
					payload.t = closerHitSphere.t2;
					payload.point = ray.GetPointAt(payload.t);
					payload.normal = closerHitSphere.object->GetPosition() - payload.point;
					payload.normal.Normalize();
					payload.frontFace = true;
					payload.material = &material;
					payload.object = this;
					return true;
				}
			}
			else {
				if (payloadCenter.frontFace && closerHitSphere.t < payloadCenter.t2) {
					// Came from completely inside outer sphere and hit lens.
					payload.t = closerHitSphere.t;
					payload.point = ray.GetPointAt(payload.t);
					payload.normal = closerHitSphere.object->GetPosition() - payload.point;
					payload.normal.Normalize();
					payload.frontFace = true;
					payload.material = &material;
					payload.object = this;
					return true;
				}
				if (!payloadCenter.frontFace && closerHitSphere.t < payloadCenter.t) {
					// Came from overlap between outer and center spheres.
					payload.t = closerHitSphere.t;
					payload.point = ray.GetPointAt(payload.t);
					payload.normal = closerHitSphere.object->GetPosition() - payload.point;
					payload.normal.Normalize();
					payload.frontFace = true;
					payload.material = &material;
					payload.object = this;
					return true;
				}
			}

			if (!payloadCenter.frontFace && closerHitSphere.frontFace && closerHitSphere.t < payloadCenter.t) {
				// Completely inside the lens.
				payload.t = closerHitSphere.t;
				payload.point = ray.GetPointAt(payload.t);
				payload.normal = closerHitSphere.object->GetPosition() - payload.point;
				payload.normal.Normalize();
				payload.frontFace = false;
				payload.material = &material;
				payload.object = this;
				return true;
			}
		}

		return false;
	}

	Vector3f DivergingLens::Sample(const Vector3f& point, float& pdf, std::mt19937& rnd) {
		return Sampling::SamplePointInCone(point, mCenter->GetPosition(), mCenter->GetScale().x, pdf, rnd);
	}

	float DivergingLens::GetWidth() const {
		return mWidth;
	}

	void DivergingLens::SetWidth(float width) {
		mWidth = width;
		mDirty = true;
	}

	float DivergingLens::GetCurvature() const {
		return mCurvature;
	}

	void DivergingLens::SetCurvature(float curvature) {
		mCurvature = curvature;
		mDirty = true;
	}

}
