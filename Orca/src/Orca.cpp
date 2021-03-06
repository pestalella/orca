// Orca.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"

#include <algorithm>
#include <chrono>
#include <fstream>
#include <vector>

#include "algebra.h"
#include "BRDF.h"
#include "camera.h"
#include "geometry.h"
#include "image_handling.h"
#include "intersection.h"
#include "random.h"
#include "ray.h"
#include "scene.h"

using Orca::Hit;
using Orca::Ray;
using Orca::Vec3f;
using Orca::Vec3d;

float luminance(Vec3f imgSample)
{
    return Vec3f(0.2126f, 0.7152f, 0.0722f).dotProduct(imgSample);
}

int main()
{
    Orca::ImageHandling::initialize();

    const int imgWidth = 512, imgHeight = 512;
    StatisticsCollector sc;
    Orca::Random rng;    
    Orca::Scene scn(&sc);
    Orca::Camera cam(Vec3f(0, -9, 3.5), Vec3f(0, 0, 2), Vec3f(0, 0, 1), 45, imgWidth, imgHeight, rng);

    rng.initHalton();
    std::cout << "Raytracing..." << std::endl;

    auto t_start = std::chrono::high_resolution_clock::now();
    std::vector<Vec3f> imageBuf(imgWidth*imgHeight, 0);
    std::vector<float> variance(imgWidth*imgHeight, 0);
    std::vector<unsigned int> sampleCount(imgWidth*imgHeight, 0);

    const int minSamples = 64;
    const int maxSamples = 1000;
    for (int i = 0; i < imgHeight; ++i) {
        for (int j = 0; j < imgWidth; ++j) {
            unsigned int numSamples = 0;
            Vec3f runningAvg = 0;
            Vec3f runningVar = 0;
            bool stable = false;
            while (numSamples < minSamples || (numSamples < maxSamples && !stable)) {
                Ray r = cam.createRay(j, i);
                sc.accumEvent("CameraRayCreated");
                Vec3f curSample = scn.traceRayBidirectional(r, 6);

                Vec3f sampleD(curSample.x, curSample.y, curSample.z);
                Vec3f newAvg = (runningAvg*(float)numSamples + sampleD)*(1.0f/(numSamples+1));
                runningVar = runningVar + (sampleD - runningAvg)*(sampleD - newAvg);

                if ((runningAvg - newAvg).norm() < 0.1)
                    stable = true;

                runningAvg = newAvg;
                numSamples++;
            }
            imageBuf[i*imgWidth + j] = runningAvg;
            variance[i*imgWidth + j] = runningVar.length()/numSamples;
            sampleCount[i*imgWidth + j] = numSamples;
        }
    }
    auto t_end = std::chrono::high_resolution_clock::now();
    double rt_time = std::chrono::duration<double, std::milli>(t_end-t_start).count();

    std::cout << "Tone mapping..." << std::endl;
    float minLum(1E20f), maxLum(1);
    float maxVariance(0);
    for (int i = imgHeight - 1; i >= 0; i--) {
        for (int j = 0; j < imgWidth; j++) {
            float lum = luminance(imageBuf[i*imgWidth + j]);
            minLum = std::min(minLum, lum);
            maxLum = std::max(maxLum, lum);
            maxVariance = std::max(maxVariance, variance[i*imgWidth + j]);
        }
    }
    std::cout << "Max stddev:" << maxVariance << std::endl;

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

    std::vector<unsigned char> outVariance(imgHeight*imgWidth);
    std::vector<unsigned char> outSampleCount(imgHeight*imgWidth);
    p = &outVariance[0];
    unsigned char *q = &outSampleCount[0];
    for (int i = imgHeight-1; i >= 0; i--) {
        for (int j = 0; j < imgWidth; j++) {
            *p++ = int(255.99*variance[i*imgWidth + j]/maxVariance);
            *q++ = int(255.99*sampleCount[i*imgWidth + j]/maxSamples);
        }
    }

    //if (imgWidth == 5) {
    //    for (int i = imgHeight-1; i >= 0; i--) {
    //        for (int j = 0; j < imgWidth; j++) {
    //            Vec3f col = imageBuf[i*imgWidth + j];
    //            std::cout << col << " ";
    //        }
    //        std::cout << std::endl;
    //    }
    //}

    std::cout << "Writing image to disk" << std::endl;
    Orca::ImageHandling::saveRGBImage(&outImg[0], imgWidth, imgHeight, "out.png");
    Orca::ImageHandling::saveLImage(&outVariance[0], imgWidth, imgHeight, "variance.png");
    Orca::ImageHandling::saveLImage(&outSampleCount[0], imgWidth, imgHeight, "sample_count.png");

    std::cout << "Statistics" << std::endl;
    std::cout << "==========" << std::endl;
    std::cout << "RT cpu time:" << rt_time/1000.0 << " seconds" << std::endl;
    std::cout << "Avg rays/pixel:" << sc.eventCount("CameraRayCreated")/(float)(imgWidth*imgHeight) << std::endl;
    sc.dump();

    return 0;
}

