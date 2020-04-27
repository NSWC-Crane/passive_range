#define _CRT_SECURE_NO_WARNINGS

#include <cstdint>

#include "sn_lib.h"

#include "simplex_noise.h"

//----------------------------------------------------------------------------------
// DLL internal state variables:
open_simplex_noise sn;


//----------------------------------------------------------------------------------
void init(long seed)
{
    sn.init(seed);
}

//----------------------------------------------------------------------------------
unsigned int evaluate(double x, double y, double scale, unsigned int num)
{
    double v = sn.evaluate((x * scale), (y * scale));
    //unsigned int index = 
    return (unsigned int)(((v + 1.0) / 2.0) * num);
    //test1(r, c) = color[index];
    //test1(r, c) = index;
}

//----------------------------------------------------------------------------------
unsigned int octave_evaluate(double x, double y, double scale, unsigned int octaves, double persistence)
{
    //num *= 5;

    double v = sn.octave(x * scale, y * scale, octaves, persistence * 1.0);
    unsigned int index = (unsigned int)(((v + 1.0) / 2.0) * 20);

    if (index < 8)
        //octave_image.at<cv::Vec3b>(r, c) = color[0];
        return 0;
    else if (index >= 8 && index < 10)
        //octave_image.at<cv::Vec3b>(r, c) = color[1];
        return 1;
    else if (index >= 10 && index < 12)
        //octave_image.at<cv::Vec3b>(r, c) = color[2];
        return 2;
    else
        //octave_image.at<cv::Vec3b>(r, c) = color[3];
        return 3;
}



