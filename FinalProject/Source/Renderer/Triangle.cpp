#include "Triangle.h"

#include <vector>
#include <Renderer/Object.h>
#include <Maths/Vector3f.h>
#include <Maths/Matrix4x4f.h>

namespace rtos {

Triangle::Triangle(Vector3f position, Vector3f verticies[3], Material material) :
	Object(position, material, ObjectType::Triangle)
{
	mVerticies[0] = verticies[0];
	mVerticies[1] = verticies[1];
	mVerticies[2] = verticies[2];

    mTransformedVerticies[0] = verticies[0];
    mTransformedVerticies[1] = verticies[1];
    mTransformedVerticies[2] = verticies[2];
}

void Triangle::Update() {
    if (mDirty) {
        // Apply transformation matrix
        mTransform = Matrix4x4f::Translate(mPosition) * Matrix4x4f::Rotate(mRotation) * Matrix4x4f::Scale(mScale);
        mTransformedVerticies[0] = mTransform * mVerticies[0];
        mTransformedVerticies[1] = mTransform * mVerticies[1];
        mTransformedVerticies[2] = mTransform * mVerticies[2];
        mDirty = false;
    }
}

bool Triangle::Intersect(const Ray& ray, float min, float max, RayPayload& payload) {
    Vector3f E1 = mTransformedVerticies[1] - mTransformedVerticies[0];
    Vector3f E2 = mTransformedVerticies[2] - mTransformedVerticies[0];
    Vector3f T = ray.GetOrigin() - mTransformedVerticies[0];
    Vector3f D = ray.GetDirection();

    Vector3f P = D.Cross(E2); 
    Vector3f Q = T.Cross(E1);

    // Don't cause a divide by 0 error!
    if (abs(P.Dot(E1)) < 0.00001) return false;

    Vector3f tuv = (1 / P.Dot(E1)) * Vector3f(Q.Dot(E2), P.Dot(T), Q.Dot(D));

    float t = tuv[0];
    float u = tuv[1];
    float v = tuv[2];

    // Must lie within the ray range
    if (t < min || t > max) return false;

    // u and v must obey barycentric co-ordinates
    if (u < 0) return false;
    if (v < 0) return false;
    if (u + v > 1) return false;

    // Normal is always this due to vertex order in obj file
    Vector3f N = E1.Cross(E2);

    // Hit the triangle!
    payload.t = t;
    payload.point = ray.GetPointAt(payload.t);
    payload.normal = N;
    payload.normal.Normalize();
    payload.frontFace = payload.normal.Dot(ray.GetDirection()) > 0 ? false : true;
    payload.material = &material;
    payload.object = this;

    return true;
}

}