// Orca.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <algorithm>
#include <vector>
#include <fstream>

#include "algebra.h"
#include "BRDF.h"
#include "camera.h"
#include "geometry.h"
#include "image_handling.h"
#include "intersection.h"
#include "ray.h"
#include "scene.h"

using Orca::Hit;
using Orca::Ray;
using Orca::Vec3f;

float luminance(Vec3f imgSample)
{
    return Vec3f(0.2126f, 0.7152f, 0.0722f).dotProduct(imgSample);
}

int main()
{
    Orca::ImageHandling::initialize();

    const int imgWidth = 500, imgHeight = 500;
    Orca::Scene scn;
    Orca::Camera cam(Vec3f(-2, -15, 4), Vec3f(0, 0, 2), Vec3f(0,0,1), 30, imgWidth, imgHeight);

    std::cout << "Raytracing..." << std::endl;
    std::vector<Vec3f> imageBuf(imgWidth*imgHeight, 0);
    for (int i = 0; i < imgHeight; ++i) {
        for (int j = 0; j < imgWidth; ++j) {
            int ns = 1000;
            for (int s = 0; s < ns; ++s) {
                Ray r = cam.createRay(j, i);
//                imageBuf[i*imgWidth + j] += scn.traceRayRecursive(r, 1);
                imageBuf[i*imgWidth + j] += scn.traceRayBidirectional(r, 4);
            }
            imageBuf[i*imgWidth + j] *= 1.0f / ns;
        }
    }

    std::cout << "Tone mapping..." << std::endl;
    float minLum(1E20f), maxLum(1);
    for (int i = imgHeight - 1; i >= 0; i--) {
        for (int j = 0; j < imgWidth; j++) {
            float lum = luminance(imageBuf[i*imgWidth + j]);
            minLum = std::min(minLum, lum);
            maxLum = std::max(maxLum, lum);
        }
    }

    std::vector<unsigned char> outImg(imgHeight*imgWidth * 3);
    unsigned char *p = &outImg[0];
    for (int i = imgHeight-1; i >= 0; i--) {
        for (int j = 0; j < imgWidth; j++) {
            Vec3f col = imageBuf[i*imgWidth + j];
            col = col * (1.0f / maxLum);
            col = Vec3f(std::min(1.0f, sqrt(col[0])), 
                        std::min(1.0f, sqrt(col[1])), 
                        std::min(1.0f, sqrt(col[2])));
            unsigned char ir = int(255.99*col[0]);
            unsigned char ig = int(255.99*col[1]);
            unsigned char ib = int(255.99*col[2]);
            *p++ = ir;
            *p++ = ig;
            *p++ = ib;
        }
    }

    if (imgWidth == 5) {
        for (int i = imgHeight-1; i >= 0; i--) {
            for (int j = 0; j < imgWidth; j++) {
                Vec3f col = imageBuf[i*imgWidth + j];
                std::cout << col << " ";
            }
            std::cout << std::endl;
        }
    }

    std::cout << "Writing image to disk" << std::endl;
    Orca::ImageHandling::saveRGBImage(&outImg[0], imgWidth, imgHeight, "out.png");

    return 0;
}

