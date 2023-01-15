#pragma once

#include <Renderer/Object.h>
#include <Renderer/Sphere.h>
#include <Maths/Ray.h>

namespace rtos {

	class ConvergingLens : Object {
	public:
		                        ConvergingLens(Vector3f position, float width, Material material);
		virtual nlohmann::json  ToJSON() override;
							    
		virtual void            Update() override;
							    
		virtual bool            Intersect(const Ray& ray, float min, float max, RayPayload& payload) override;
		virtual Vector3f        Sample(const Vector3f& point, float& pdf, std::mt19937& rnd) override;

		float                   GetWidth() const;
		void                    SetWidth(float width);
							    
	private:			
		float                   mWidth;
		Object*                 mSphere1;
		Object*                 mSphere2;
	};

}