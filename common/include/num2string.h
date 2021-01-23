#ifndef NUM_TO_STRING_H_
#define NUM_TO_STRING_H_

#define _CRT_SECURE_NO_WARNINGS

#include <cstdio>
#include <cstdlib>
#include <cstdint>
#include <string>

template<typename T>
std::string num2str(T val, std::string fmt)
{
    char in_string[64];
    sprintf(in_string, fmt.c_str(), val);
    return std::string(in_string);  
}   // end of num2str    
    
#endif  // NUM_TO_STRING_H_
