#pragma once

#include <Maths/Vector2f.h>
#include <Maths/Vector3f.h>
#include <Maths/Ray.h>
#include <Renderer/RayPayload.h>
#include <random>

#include <nlohmann/json.hpp>

namespace Prototype {

enum class MaterialType {
    Lambertian,
    Glossy,
    Glass
};

enum class MaterialPreset {
    Paper,
    Plastic,
    Metal,
    Glass,
    FrostedGlass,
    Light
};

class Material {
public:
                        Material(MaterialType type = MaterialType::Lambertian, Colour colour = Vector3f(0.9, 0.9, 0.9), float roughness = 1, float refractiveIndex = 1.5, float emitted = 0);

    bool                operator==(const Material& material) const;

    static Material     LoadFromPreset(MaterialPreset preset);

    static Material     LoadFromJSON(nlohmann::json data);
    nlohmann::json      ToJSON();

    bool                Scatter(const Ray& incoming, Ray& out, float& pdf, const RayPayload& payload, std::mt19937& rnd); // Determines if a ray will scatter.
    Colour              Emit();                                                                                           // The colour emitted if the material emits light.

    float               GetPDF(const Vector3f& direction, const RayPayload& payload);

    MaterialType        materialType;
    Colour              colour;
    float               roughness;
    float               refractiveIndex;
    float               emitted;

private:
    Vector3f            Reflect(Vector3f d, Vector3f n);
    Vector3f            Refract(Vector3f d, Vector3f n, float ratio);
    float               RSchlick2(Vector3f d, Vector3f n, float ir1, float ir2); // Used to approximate reflectance.
    //Vector3f            RandomInUnitSphere(std::mt19937& rnd);
    //Vector3f            RandomInUnitHemisphere(Vector3f normal, std::mt19937& rnd);
};

}