#pragma once

#include <random>

#include <Maths/Vector2f.h>
#include <Maths/Vector3f.h>

namespace Prototype {

namespace Sampling {
    Vector2f            SampleBilinear(std::mt19937& rnd);
    Vector3f            SampleDirectionInHemisphere(const Vector3f& normal, std::mt19937& rnd);
    Vector3f            SampleDirectionInPhong(const Vector3f& direction, float s, std::mt19937& rnd);
    Vector3f            TransformSample(const Vector3f& sampleRelativeToTheZAxes, const Vector3f& direction);
    Vector3f            SamplePointInCone(const Vector3f& point, const Vector3f& center, float radius, float& pdf, std::mt19937& rnd);
}

}