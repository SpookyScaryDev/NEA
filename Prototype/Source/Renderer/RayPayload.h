#pragma once

#include <Maths/Vector3f.h>

namespace Prototype {

class Material;
class Object;

struct RayPayload {
    Vector3f            point;
    Vector3f            normal;
    bool                frontFace;
    float               t;
    Material*           material;
    Object*             object;
};

}