#include "Sampling.h"

#define _USE_MATH_DEFINES
#include <math.h>
#include <random>

#include <Maths/Vector2f.h>
#include <Maths/Vector3f.h>

namespace Prototype {

namespace Sampling {
    Vector2f SampleBilinear(std::mt19937& rnd) {
        std::uniform_real_distribution<float> dist(0.0, 1.0);
        return { dist(rnd), dist(rnd) };
    }

    Vector3f SampleDirectionInHemisphere(const Vector3f& normal, std::mt19937& rnd) {
        Vector2f bilinearDistribution = SampleBilinear(rnd);
        //float a = 1 - 2 * bilinearDistribution.x;
        //float b = sqrt(1 - a * a);
        //float phi = 2 * M_PI * bilinearDistribution.y;
        //Vector3f direction = Vector3f();
        //direction.x = normal.x + b * cos(phi);
        //direction.y = normal.y + b * sin(phi);
        //direction.z = normal.z + a;
        //direction.Normalize();

        Vector3f direction = Vector3f();
        direction.x = sqrt(bilinearDistribution[0]) * cos(2 * M_PI * bilinearDistribution[1]);
        direction.y = sqrt(bilinearDistribution[0]) * sin(2 * M_PI * bilinearDistribution[1]);
        direction.z = sqrt(1 - bilinearDistribution[0]);
        direction.Normalize();
        if (direction.Dot(Vector3f(0, 0, 1)) <= 0) direction = Vector3f(0, 0, 1);

        return TransformSample(direction, normal);
    }

    Vector3f SampleDirectionInPhong(const Vector3f& direction, float s, std::mt19937& rnd) {
        Vector3f out = { 0, 0, -1 };
        //while (out.Dot(Vector3f(0, 0, 1)) < 0) {
        Vector2f bilinearDistribution = SampleBilinear(rnd);
        float cosTheta = pow(1 - bilinearDistribution[0], 1 / (1 + s));
        float sinTheta = sqrt(1 - cosTheta * cosTheta);
        float phi = 2 * M_PI * bilinearDistribution[1];
        out.x = cos(phi) * sinTheta;
        out.y = sin(phi) * sinTheta;
        out.z = cosTheta;
        //}
        return TransformSample(out, direction);
    }

    Vector3f TransformSample(const Vector3f& sampleRelativeToTheZAxes, const Vector3f& direction) {
        // A is some vector which is not parallel to any of the ONB axes.
        Vector3f a = { 1, 0, 0 };
        if (direction.x > 0.9) a = { 0, 1, 0 }; // Make sure a is not parallel to direction.

        Vector3f s = a.Cross(direction);
        s.Normalize();
        Vector3f t = s.Cross(direction);
        t.Normalize();

        return sampleRelativeToTheZAxes.x * s + sampleRelativeToTheZAxes.y * t + sampleRelativeToTheZAxes.z * direction;
    }

    Vector3f SamplePointInCone(const Vector3f& point, const Vector3f& center, float radius, float& pdf, std::mt19937& rnd) {
        Vector3f toCenter = center - point;
        Vector3f randomRay = Vector3f(1, 0, 0);
        if (randomRay == toCenter) randomRay = Vector3f(0, 1, 0);
        Vector3f perpendicular = toCenter.Cross(randomRay);
        perpendicular.Normalize();
        Vector3f toEdge = toCenter + radius * perpendicular;
        toCenter.Normalize();
        toEdge.Normalize();
        float cosThetaMax = toCenter.Dot(toEdge);

        Vector2f u = Sampling::SampleBilinear(rnd);
        float cosTheta = (1 - u[0]) + u[0] * cosThetaMax;
        float sinTheta = sqrt(1 - cosTheta * cosTheta);
        float phi = u[1] * 2 * M_PI;
        Vector3f direction;
        direction.x = cos(phi) * sinTheta;
        direction.y = sin(phi) * sinTheta;
        direction.z = cosTheta;

        pdf = 1 / (2 * M_PI * (1 - cosThetaMax));

        return Sampling::TransformSample(direction, toCenter);
    }
}

}