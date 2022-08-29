#pragma once

#include <Maths/Vector3f.h>

namespace Prototype {

class Material;
class Object;

// Stores details about the intersection between a ray and an object.
struct RayPayload {
    Vector3f            point;
    Vector3f            normal;
    bool                frontFace;
    float               t;
    Material*           material;
    Object*             object;
};

}