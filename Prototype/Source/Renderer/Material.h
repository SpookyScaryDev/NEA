#pragma once

#include <Maths/Vector3f.h>
#include <Maths/Ray.h>
#include <Renderer/RayPayload.h>

namespace Prototype {

enum class MaterialType {
    Lambertian,
    Glass
};

class Material {
public:
                        Material(MaterialType type, Colour colour, Colour emitted = { 0, 0, 0 });

    bool                Scatter(const Ray& incoming, Ray& out, const RayPayload& payload);
    Colour              Emit();

    MaterialType        materialType;
    Colour              colour;
    Colour              emitted;

private:
    Vector3f            RandomInUnitSphere();
    Vector3f            RandomInUnitHemisphere(Vector3f normal);
};

}