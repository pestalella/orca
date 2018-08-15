// Orca.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#define _USE_MATH_DEFINES
#include <cmath>
#include <vector>
#include <fstream>

#include "geometry.h"

class Ray
{
public:
    Vec3f origin;
    Vec3f dirNorm;

    Ray(Vec3f org, Vec3f dir) :
        origin(org), dirNorm(dir)
    {}
};

class Hit
{
public:
    Vec3f pos;
    Vec3f normal;
    Vec3f color;
    float t;  // hit distance along the ray
    bool miss;

    Hit(Vec3f pos, Vec3f normal, Vec3f color, float t) :
        pos(pos), normal(normal), color(color), t(t), miss(false)
    {}

    Hit() :
        pos(0), normal(0), color(0), t(0), miss(true)
    {}
};

class IntersectableObject
{
public:
    virtual Hit intersect(const Ray &r) = 0;
};

class Ball : public IntersectableObject
{
private:
    Vec3f origin;
    float radius;
    float radius2;
    Vec3f color;

public:
    Ball(Vec3f org, float radius, Vec3f color) :
        origin(org), radius(radius), radius2(radius*radius), color(color)
    {}

    Hit intersect(const Ray &r);
};

class Plane : public IntersectableObject
{
};

class Camera
{
private:
    Vec3f origin;
    Vec3f up;
    Vec3f viewDir;
    float fov;
    int widthPixels, heightPixels;
    Matrix44f cameraToWorld;

public:
    Camera(Vec3f org, Vec3f lookat, float fov, int width, int height) :
        origin(org), up(0,0,1), viewDir(lookat-org), fov(fov), widthPixels(width), 
        heightPixels(height)
    {
        Vec3f right = viewDir.crossProduct(up);
        up = right.crossProduct(viewDir);

        std::cout << "Viewdir:" << viewDir << " right:" << right << "  up:" << up << std::endl;

        Matrix44f world2cam = Matrix44f(
            right.x, right.y, right.z, 0,
            viewDir.x, viewDir.y, viewDir.z, 0,
            up.x,      up.y,      up.z, 0,
            origin.x,  origin.y,  origin.z, 1);
        cameraToWorld = world2cam.transposed();
    }

    Ray createRay(int i, int j)
    {
        float scale = tan(M_PI*(fov * 0.5)/180.0);
        float imageAspectRatio = widthPixels / (float)heightPixels;
        Vec3f orig;
        cameraToWorld.multVecMatrix(Vec3f(0), orig);
        float x = (2 * (i + 0.5) /   (float)widthPixels - 1) * imageAspectRatio * scale;
        float y = (1 - 2 * (j + 0.5) / (float)heightPixels) * scale;
        Vec3f dir;
        cameraToWorld.multDirMatrix(Vec3f(x, y, 1), dir);
        dir.normalize();
        return Ray(origin, dir);
    }
};

class Scene
{
private:
    std::vector<IntersectableObject *> objects;

public:
    Scene()
    {
        objects.push_back(new Ball(Vec3f(-2, -2, 2), 0.5, Vec3f(0, 0, 0)));
        objects.push_back(new Ball(Vec3f(2, -2, 2), 0.5,  Vec3f(0, 0, 1)));
        objects.push_back(new Ball(Vec3f(-2, 2, 2), 0.5,  Vec3f(0, 1, 0)));
        objects.push_back(new Ball(Vec3f(2, 2, 2), 0.5,   Vec3f(0, 1, 1)));

        objects.push_back(new Ball(Vec3f(-2, -2, -2), 0.5, Vec3f(1, 0, 0)));
        objects.push_back(new Ball(Vec3f(2, -2, -2), 0.5,  Vec3f(1, 0, 1)));
        objects.push_back(new Ball(Vec3f(-2, 2, -2), 0.5,  Vec3f(1, 1, 0)));
        objects.push_back(new Ball(Vec3f(2, 2, -2), 0.5,   Vec3f(1, 1, 1)));
    }
    Hit intersectClosest(const Ray &r);
};

bool solveQuadratic(const float &a, const float &b, const float &c, float &x0, float &x1)
{
    float discr = b * b - 4 * a * c;
    if (discr < 0) return false;
    else if (std::abs(discr) < 1E-6) x0 = x1 = -0.5 * b / a;
    else {
        float q = (b > 0) ?
            -0.5 * (b + sqrt(discr)) :
            -0.5 * (b - sqrt(discr));
        x0 = q / a;
        x1 = c / q;
    }
    if (x0 > x1) std::swap(x0, x1);

    return true;
}

Hit Ball::intersect(const Ray & r)
{
    float t0, t1; // solutions for t if the ray intersects 
    // analytic solution
    Vec3f L = r.origin - origin;
    float a = 1.0;
    float b = 2 * r.dirNorm.dotProduct(L);
    float c = L.dotProduct(L) - radius2;
    if (!solveQuadratic(a, b, c, t0, t1)) return Hit();
    if (t0 > t1) std::swap(t0, t1);

    if (t0 < 0) {
        t0 = t1; // if t0 is negative, let's use t1 instead 
        if (t0 < 0) return Hit(); // both t0 and t1 are negative 
    }

    float t = t0;

    Vec3f hitPoint = r.origin + r.dirNorm*t;
    Vec3f hitNormal = hitPoint - origin;
    hitNormal.normalize();
    return Hit(hitPoint, hitNormal, color, t);
}

Hit Scene::intersectClosest(const Ray & r)
{
    Hit closestHit;
    for (auto curObj : objects) {
        Hit curHit = curObj->intersect(r);
        if (!curHit.miss) {
            if (closestHit.miss || curHit.t < closestHit.t) {
                closestHit = curHit;
            }
        }
    }
    return closestHit;
}

int main()
{
    const int imgWidth = 500, imgHeight = 500;
    std::vector<Vec3f> imageBuf(imgWidth*imgHeight);

    Scene scn;
    Camera cam(Vec3f(0, 0, 0), Vec3f(1, 0, 0), 90, imgWidth, imgHeight);

    for (int i = 0; i < imgHeight; ++i) {
        for (int j = 0; j < imgWidth; ++j) {
            Hit hInfo = scn.intersectClosest(cam.createRay(j, i));
            if (hInfo.miss) {
                imageBuf[i*imgWidth + j] = Vec3f(1, 0, 0);
            } else {
                imageBuf[i*imgWidth + j] = hInfo.color;
            }
        }
    }
    std::ofstream outFile("out.ppm");

    outFile << "P3\n" << imgWidth << " " << imgHeight << "\n255\n";

    for (int i = 0; i < imgHeight; i++) {
        for (int j = 0; j < imgWidth; j++) {
            Vec3f col = imageBuf[i*imgWidth + j];
//            col = Vec3f(sqrt(col[0]), sqrt(col[1]), sqrt(col[2]));
            int ir = int(255.99*col[0]);
            int ig = int(255.99*col[1]);
            int ib = int(255.99*col[2]);
            outFile << ir << " " << ig << " " << ib << "\n";
        }
    }
    return 0;
}
