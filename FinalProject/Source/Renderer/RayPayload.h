#pragma once

#include <Maths/Vector3f.h>

namespace rtos {

class Material;
class Object;

// Stores details about the intersection between a ray and an object.
struct RayPayload {
    Vector3f            point;      // The point in 3D space where the intersection occured.
    Vector3f            normal;     // The normal to the surface at the point of intersection. Always points outwards.
    bool                frontFace;  // True if the ray came intersected from outside the object.
    float               t;          // The point along the ray where the interseciton occured.
    float               t2;
    Material*           material;
    Object*             object;
};

}