#pragma once

#include <Renderer/Object.h>
#include <Renderer/Sphere.h>
#include <Maths/Ray.h>

namespace rtos {

	class DivergingLens : Object {
	public:
		                        DivergingLens(Vector3f position, float width, float curvature, Material material);
		virtual nlohmann::json  ToJSON() override;
							    
		virtual void            Update() override;
							    
		virtual bool            Intersect(const Ray& ray, float min, float max, RayPayload& payload) override;
		virtual Vector3f        Sample(const Vector3f& point, float& pdf, std::mt19937& rnd) override;

		float                   GetWidth() const;
		void                    SetWidth(float width);

		float                   GetCurvature() const;
		void                    SetCurvature(float curvature);
							    
	private:				  
		float                   mWidth;
		float                   mCurvature;

		Object*                 mCenter;
		Object*                 mSphere1;
		Object*                 mSphere2;
	};

}