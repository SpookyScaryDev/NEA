#include "Material.h"

#define _USE_MATH_DEFINES
#include <math.h>
#include <Maths/Vector2f.h>
#include <Maths/Vector3f.h>
#include <Maths/Ray.h>
#include <Renderer/RayPayload.h>
#include <cstdlib>

namespace Prototype {

Material::Material(MaterialType type, Colour colour, float roughness, float refractiveIndex, float emitted) :
    materialType(type),
    colour(colour),
    roughness(roughness),
    refractiveIndex(refractiveIndex),
    emitted(emitted) {}

bool Material::Scatter(const Ray& incoming, Ray& out, float& pdf, const RayPayload& payload, std::mt19937& rnd) {
    std::uniform_real_distribution<float> dist(0.0, 1.0);

    switch (materialType) {

    case MaterialType::Lambertian: {
        // Reflects in any direction.
        Vector3f normal = payload.normal;
        if (!payload.frontFace) normal *= -1;
        //Vector3f direction = RandomInUnitHemisphere(normal, rnd) + 0.1 * normal;

        Vector3f direction = SampleDirectionInHemisphere(normal, rnd);
        pdf = DirectionInHemispherePDF(direction, normal);

        out = Ray(payload.point, direction);
        break;
    }
    case MaterialType::Glossy: {
        // Reflects according to Snell's law, with randomization determined by roughness.
        Vector3f direction = Reflect(incoming.GetDirection(), payload.normal);
        direction += roughness * RandomInUnitHemisphere(direction, rnd);
        direction.Normalize();
        out = Ray(payload.point, direction);
        break;
    }
    case MaterialType::Glass: {
        float ratio;
        // For now, assume that the ray is coming from / leaving the air. Air has a refractive index of 1.
        if (payload.frontFace) ratio = 1.0 / refractiveIndex;
        else ratio = refractiveIndex / 1.0;

        // Make sure normal is pointing the right way! The normal always points outwards from the object,
        // so if the may is coming from inside the object, the normal used to calculate refraction / reflection
        // must be flipped
        Vector3f normal = payload.normal;
        if (!payload.frontFace)
            normal *= -1;

        float reflectance;
        if (payload.frontFace) reflectance = RSchlick2(incoming.GetDirection(), normal, 1, refractiveIndex);
        else reflectance = RSchlick2(incoming.GetDirection(), normal, refractiveIndex, 1);

        Vector3f direction;
        if (reflectance > dist(rnd)) {
            direction = Reflect(incoming.GetDirection(), normal);
        }
        else {
            direction = Refract(incoming.GetDirection(), normal, ratio);
        }
         
        out = Ray(payload.point, direction + roughness * RandomInUnitHemisphere(direction, rnd));
        break;
    }
    default:
        break;
    }
    
    return true;
}

Colour Material::Emit() {
    return emitted * colour;
}

Vector3f Material::Reflect(Vector3f i, Vector3f n) {
    return i - 2 * (i.Dot(n)) * n;
}

Vector3f Material::Refract(Vector3f i, Vector3f n, float ratio) {
    float cosTheta_i = (-1 * i).Dot(n);
    float sin2Theta_t = ratio * ratio * (1 - cosTheta_i * cosTheta_i);
    Vector3f refracted = ratio * i + (ratio * cosTheta_i - sqrt(1 - sin2Theta_t)) * n;
    return refracted;
}

float Material::RSchlick2(Vector3f i, Vector3f n, float ir1, float ir2) {
    float r0 = (ir1 - ir2) / (ir1 + ir2);
    r0 *= r0;
    float cosTheta_i = -1 *  n.Dot(i);
    float ratio = ir1 / ir2;
    float sin2Theta_t = ratio * ratio * (1.0 - cosTheta_i * cosTheta_i);
    bool tir = sin2Theta_t > 1.0;
    if (ir1 <= ir2) {
        float x = 1.0 - cosTheta_i;
        return r0 + (1 - r0) * x * x * x * x * x;
    }
    else if (ir1 > ir2 && !tir) {
        float cosTheta_t = sqrt(1.0 - sin2Theta_t);
        float x = 1.0 - cosTheta_t;
        return r0 + (1.0 - r0) * x * x * x * x * x;
    }
    else {
        return 1;
    }

    //float cosine = fmin((-1 * i).Dot(n), 1.0);
    //float ref_idx = ir1 / ir2;

    //auto r0 = (1 - ref_idx) / (1 + ref_idx);
    //r0 = r0 * r0;
    //return r0 + (1 - r0) * pow((1 - cosine), 5);

}

Vector3f Material::RandomInUnitSphere(std::mt19937& rnd) {
    std::uniform_real_distribution<float> dist(0.0, 1.0);

    Vector3f direction;
    do {
        // Generate a random position in the unit cube centered about (0, 0, 0).
        direction.x = dist(rnd) - 0.5;
        direction.y = dist(rnd) - 0.5;
        direction.z = dist(rnd) - 0.5;
    }
    // Reject if the length is greater than 1 (not in the unit sphere).
    while (direction.Magnitude() > 1);
    return direction;
}

Vector3f Material::RandomInUnitHemisphere(Vector3f normal, std::mt19937& rnd) {
    Vector3f direction = RandomInUnitSphere(rnd);
    // If not in the hemisphere, flip.
    if (direction.Dot(normal) < 0) direction = direction * -1;
    return direction;
}

Vector2f Material::SampleBilinear(std::mt19937& rnd) {
    std::uniform_real_distribution<float> dist(0.0, 1.0);
    return { dist(rnd), dist(rnd) };
}

Vector3f Material::SampleDirectionInHemisphere(const Vector3f& normal, std::mt19937& rnd) {
    Vector2f bilinearDistribution = SampleBilinear(rnd);
    float a = 1 - 2 * bilinearDistribution.x;
    float b = sqrt(1 - a * a);
    float phi = 2 * M_PI * bilinearDistribution.y;
    Vector3f direction = Vector3f();
    direction.x = normal.x + b * cos(phi);
    direction.y = normal.y + b * sin(phi);
    direction.z = normal.z + a;
    direction.Normalize();
    return direction;
}

float Material::DirectionInHemispherePDF(const Vector3f& direction, const Vector3f& normal) {
    float a = direction.z - normal.z;
    return fabs(a / M_PI);
}

}