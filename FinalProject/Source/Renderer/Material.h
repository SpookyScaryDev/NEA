#pragma once

#include <Maths/Vector3f.h>
#include <Maths/Ray.h>
#include <Renderer/RayPayload.h>

namespace Prototype {

enum class MaterialType {
    Lambertian,
    Glossy,
    Glass
};

class Material {
public:
                        Material(MaterialType type = MaterialType::Lambertian, Colour colour = Vector3f(), float roughness = 1, float refractiveIndex = 1.5, Colour emitted = { 0, 0, 0 });

    bool                Scatter(const Ray& incoming, Ray& out, const RayPayload& payload); // Determines if a ray will scatter.
    Colour              Emit();                                                            // The colour emitted if the material emits light.

    MaterialType        materialType;
    Colour              colour;
    float               roughness;
    float               refractiveIndex;
    Colour              emitted;

private:
    Vector3f            Reflect(Vector3f d, Vector3f n);
    Vector3f            Refract(Vector3f d, Vector3f n, float ratio);
    float               RSchlick2(Vector3f d, Vector3f n, float ir1, float ir2); // Used to approximate reflectance.
    Vector3f            RandomInUnitSphere();
    Vector3f            RandomInUnitHemisphere(Vector3f normal);
};

}