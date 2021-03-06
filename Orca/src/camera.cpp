#include "stdafx.h"
#define _USE_MATH_DEFINES
#include <cmath>

#include "camera.h"
#include "random.h"

namespace Orca {

    Camera::Camera(Vec3f const & org, Vec3f const & lookat, Vec3f const & requestedUp, float fov, int width, int height, Random &rng) :
        origin(org), up(requestedUp), viewDir(lookat - org), fov(fov),
        widthPixels(width), heightPixels(height), rng(rng)
    {
        viewDir.normalize();
        Vec3f right = viewDir.crossProduct(up);
        up = right.crossProduct(viewDir);

        cameraToWorld = Matrix44f(
            right.x, right.y, right.z, 0,
            up.x, up.y, up.z, 0,
            -viewDir.x, -viewDir.y, -viewDir.z, 0,
            origin.x, origin.y, origin.z, 1);

        std::cout << "0,0:" << createRay(0, 0).dirNorm << std::endl;
        std::cout << width << ",0:" << createRay(width, 0).dirNorm << std::endl;
        std::cout << width << "," << height << ":" << createRay(width, height).dirNorm << std::endl;
        std::cout << "0," << height << ":" << createRay(0, height).dirNorm << std::endl;
    }

    Ray Camera::createRay(int i, int j)
    {
        float scale = (float)tan(M_PI*(fov * 0.5) / 180.0);
        float imageAspectRatio = widthPixels / (float)heightPixels;
        Vec2f curRand = rng.uniform01Halton();
        float x = (2.0f*(i + curRand.x) / (float)widthPixels - 1)*imageAspectRatio*scale;
        float y = (1.0f - 2.0f*(j + curRand.y) / (float)heightPixels)*scale;
        Vec3f dir;
        cameraToWorld.multDirMatrix(Vec3f(x, y, -1), dir);
        dir.normalize();
        return Ray(origin, dir);
    }
}