#ifndef SN_LIB_H
#define SN_LIB_H

#if defined(_WIN32) | defined(__WIN32__) | defined(__WIN32) | defined(_WIN64) | defined(__WIN64)

#if defined(BUILD_LIB)

#ifdef SN_DLL_EXPORTS
#define SN_DLL_API __declspec(dllexport)
#else
#define SN_DLL_API __declspec(dllimport)
#endif

#else

#define SN_DLL_API

#endif

#else

#define SN_DLL_API

#endif


struct layer_struct
{
    unsigned int k;
    unsigned int n;
    unsigned int nr;
    unsigned int nc;
    unsigned int size;
};

#ifdef __cplusplus
extern "C" {
#endif
    // This function will initialize the network and load the required weights
    SN_DLL_API void init(long seed);
#ifdef __cplusplus
}
#endif

#ifdef __cplusplus
extern "C" {
#endif
    // This function will take an grayscale image in std::vector<uint8_t> row major order
    // as an input and produce a resulting classification of the image.  The input must be 28*28
    SN_DLL_API unsigned int evaluate(double x, double y, double scale, unsigned int num);
#ifdef __cplusplus
}
#endif



#ifdef __cplusplus
extern "C" {
#endif
    SN_DLL_API unsigned int octave_evaluate(double x, double y, double scale, unsigned int octaves, double persistence)
        ;
#ifdef __cplusplus
}
#endif


#endif  // SN_LIB_H
