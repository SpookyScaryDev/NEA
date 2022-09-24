#include "Triangle.h"

#include <vector>
#include <Renderer/Object.h>
#include <Maths/Vector3f.h>

namespace Prototype {

Triangle::Triangle(Vector3f position, Vector3f verticies[3], Material material) :
	Object(position, material)
{
	mVerticies[0] = verticies[0];
	mVerticies[1] = verticies[1];
	mVerticies[2] = verticies[2];
}

bool Triangle::Intersect(const Ray& ray, float min, float max, RayPayload& payload) {
    // TODO: Go through this!
    
    //ray.GetDirection().Normalize();
    Vector3f v0 = mVerticies[0] * scale + position;
    Vector3f v1 = mVerticies[1] * scale + position;
    Vector3f v2 = mVerticies[2] * scale + position;

    Vector3f v0v1 = v1 - v0;
    Vector3f v0v2 = v2 - v0;
    Vector3f N = v0v1.Cross(v0v2);  //N 
    Vector3f pvec = ray.GetDirection().Cross(v0v2);
    float det = v0v1.Dot(pvec);
    // ray and triangle are parallel if det is close to 0
    if (fabs(det) < 0.000001) return false;
    float invDet = 1 / det;

    Vector3f tvec = ray.GetOrigin() - v0;
    float u = tvec.Dot(pvec) * invDet;
    if (u < 0 || u > 1) return false;

    Vector3f qvec = tvec.Cross(v0v1);
    float v = ray.GetDirection().Dot(qvec) * invDet;
    if (v < 0 || u + v > 1) return false;

    float t = v0v2.Dot(qvec) * invDet;
    if (t < min || t > max) return false;

    payload.t = t;
    payload.point = ray.GetPointAt(payload.t);
    payload.normal = N;
    payload.normal.Normalize();
    payload.frontFace = payload.normal.Dot(ray.GetDirection()) < 0 ? false : true;
    payload.material = &material;
    payload.object = this;

    return true;  //this ray hits the triangle 
}

}