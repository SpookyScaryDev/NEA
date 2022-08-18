#include "Material.h"

#include <Maths/Vector3f.h>
#include <Maths/Ray.h>
#include <Renderer/RayPayload.h>
#include <cstdlib>

namespace Prototype {

Material::Material(MaterialType type, Colour colour, Colour emitted) :
    materialType(type),
    colour(colour),
    emitted(emitted) {}

bool Material::Scatter(const Ray& incoming, Ray& out, const RayPayload& payload) {
    out = Ray(payload.point, RandomInUnitHemisphere(payload.normal));
    return true;
}

Colour Material::Emit() {
    return emitted;
}

Vector3f Material::RandomInUnitSphere() {
    Vector3f direction;
    do {
        direction.x = rand() / (RAND_MAX + 1.0) - 0.5;
        direction.y = rand() / (RAND_MAX + 1.0) - 0.5;
        direction.z = rand() / (RAND_MAX + 1.0) - 0.5;
    }
    while (direction.Magnitude() > 1);
    return direction;
}

Vector3f Material::RandomInUnitHemisphere(Vector3f normal) {
    Vector3f direction = RandomInUnitSphere();
    if (direction.Dot(normal) > 0) direction = direction * -1;
    return direction;
}


}