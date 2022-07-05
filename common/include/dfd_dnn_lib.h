#ifndef DFD_DNN_DLL_H_
#define DFD_DNN_DLL_H_

//#define EXTERN_C
//#include <cstdint>
//#include <string>
//#include <vector>

#if defined(_WIN32) | defined(__WIN32__) | defined(__WIN32) | defined(_WIN64) | defined(__WIN64)

#if defined(BUILD_LIB)

#ifdef LIB_EXPORTS
#define LIB_API __declspec(dllexport)
#else
#define LIB_API __declspec(dllimport)
#endif

#else

#define LIB_API

#endif

#else
#define LIB_API

#endif

// ----------------------------------------------------------------------------------------
struct layer_struct
{
    unsigned int k;
    unsigned int n;
    unsigned int nr;
    unsigned int nc;
    unsigned int size;
};


// ----------------------------------------------------------------------------------------
#ifdef __cplusplus
extern "C" {
#endif
    // This function will initialize the network and load the required weights
    LIB_API void init_net(const char *net_name, unsigned int *num_classes);
#ifdef __cplusplus
}
#endif

// ----------------------------------------------------------------------------------------
#ifdef __cplusplus
extern "C" {
#endif
    // This function will output a vector of the output layer for the final classification layer
    LIB_API void run_net(unsigned char* image_f1, unsigned char* image_f2, unsigned int nr, unsigned int nc, unsigned short* depth_map);
#ifdef __cplusplus
}
#endif

// ----------------------------------------------------------------------------------------
//#ifdef __cplusplus
//extern "C" {
//#endif
//    // This function will take an grayscale image in unsigned char row major order [r0,c0, r0,c1, r0,c2,...]
//    // as an input and will return the centers of the detections in the image
//    LIB_API void get_detections(unsigned char* input_img, unsigned int nr, unsigned int nc, unsigned int* num_dets, struct detection_struct*& dets);
//#ifdef __cplusplus
//}
//#endif

// ----------------------------------------------------------------------------------------
//#ifdef __cplusplus
//extern "C" {
//#endif
//    // This function will take an grayscale image in unsigned char row major order [r0,c0, r0,c1, r0,c2,...]
//    // as an input and will return the centers of the detections in the image
//    LIB_API void get_cropped_detections(unsigned char* input_img, unsigned int nr, unsigned int nc, unsigned int x, unsigned int y, unsigned int w, unsigned int h, unsigned int* num_dets, struct detection_struct*& dets);
//#ifdef __cplusplus
//}
//#endif

// ----------------------------------------------------------------------------------------
#ifdef __cplusplus
extern "C" {
#endif
    // This function will output a vector of the output layer for the final classification layer
    LIB_API void close_lib();
#ifdef __cplusplus
}
#endif

// ----------------------------------------------------------------------------------------
//#ifdef __cplusplus
//extern "C" {
//#endif
//    // This function will output a vector of the output layer for the final classification layer
//    //OBJ_DLL_API void get_combined_output(unsigned char* input_img, unsigned int nr, unsigned int nc, float* &data_params);
//    LIB_API void get_combined_output(struct layer_struct* data, const float*& data_params);
//#ifdef __cplusplus
//}
//#endif

// ----------------------------------------------------------------------------------------
#ifdef __cplusplus
extern "C" {
#endif
    // This function will print out the network to the screen
    LIB_API void print_net();
#ifdef __cplusplus
}
#endif

// ----------------------------------------------------------------------------------------
#ifdef __cplusplus
extern "C" {
#endif
    // This function will output a vector of the output layer for the final classification layer
    LIB_API void get_layer_01(struct layer_struct *data, const float* &data_params);
#ifdef __cplusplus
}
#endif

//// ----------------------------------------------------------------------------------------
#ifdef __cplusplus
extern "C" {
#endif
    LIB_API void get_input_layer(struct layer_struct *data, const float* &data_params);
#ifdef __cplusplus
}
#endif


#endif  // DFD_DNN_DLL_H_
