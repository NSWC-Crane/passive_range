#ifndef FILE_OPERATIONS_H_
#define FILE_OPERATIONS_H_

#if defined(_WIN32) | defined(__WIN32__) | defined(__WIN32) | defined(_WIN64) | defined(__WIN64)

#include <windows.h>

#elif defined(__linux__)

#include <limits.h>
#include <cstring>
#include <unistd.h>
#include <libgen.h>

// The system cannot find the path specified
#define ERROR_PATH_NOT_FOUND 3L
// Cannot create a file when that file already exists
#define ERROR_ALREADY_EXISTS 183L
    
#endif

#include <algorithm>
#include <vector> 
#include <iostream>
#include <sstream>
#include <fstream>
//#include <utility>
#include <cctype>
#include <string>
#include <sys/stat.h>

/*
// ----------------------------------------------------------------------------
// trim from start (in place)
static inline void ltrim(std::string &s) {
	s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](int ch) {
		return !std::isspace(ch);
	}));
}

// trim from end (in place)
static inline void rtrim(std::string &s) {
    s.erase(std::find_if(s.rbegin(), s.rend(), [](int ch) {
        return !std::isspace(ch);
    }).base(), s.end());
}

// trim from both ends (in place)
static inline void trim(std::string &s) {
	ltrim(s);
	rtrim(s);
}
*/

// ----------------------------------------------------------------------------
std::string get_path(std::string filename, std::string sep)
{
    int prog_loc = (int)(filename.rfind(sep));
    return filename.substr(0, prog_loc);    // does not return the last separator
}

// ----------------------------------------------------------------------------

#if defined(__linux__)
// this may or may not work.  There is some dicussion about elevated privileges required for readlink
std::string get_ubuntu_path()
{
    std::string path = "/";
    char result[PATH_MAX+1];
    memset(result, 0, sizeof(result));

    ssize_t count = readlink("/proc/self/exe", result, PATH_MAX);
    
    if (count != -1)
    {
        path = get_path(dirname(result), "/") + "/";
    }

    return (path);

}
#endif

// ----------------------------------------------------------------------------
std::string path_check(std::string path)
{
    if (path.empty())
        return path;

    std::string path_sep = path.substr(path.length() - 1, 1);
    if (path_sep != "\\" & path_sep != "/")
    {
        return path + "/";
    }

    return path;

}   // end of path_check

// ----------------------------------------------------------------------------
inline bool existence_check(const std::string &path)
{
    struct stat buffer;
    return (stat(path.c_str(), &buffer) == 0);
}   // end of existence_check

// ----------------------------------------------------------------------------
void get_file_parts(std::string fullfilename, std::string &file_path, std::string &file_name, std::string &file_ext)
{
    // get the extension location
    std::size_t file_ext_loc = fullfilename.rfind('.');
    std::size_t last_file_sep;

    // get the last file separator location depending on OS
#if defined(_WIN32) | defined(__WIN32__) | defined(__WIN32) | defined(_WIN64) | defined(__WIN64)
    last_file_sep = fullfilename.find_last_of("/\\");
#else
    last_file_sep = fullfilename.find_last_of("/\\");
#endif

    file_path = fullfilename.substr(0, last_file_sep);
    file_name = fullfilename.substr(last_file_sep + 1, (file_ext_loc - last_file_sep - 1));

    if (file_ext_loc > fullfilename.length())
        file_ext = "";
    else
        file_ext = fullfilename.substr(file_ext_loc, (fullfilename.length() - file_ext_loc));
    
}	// end of get_file_parts

// ----------------------------------------------------------------------------------------
void separate_paths(std::string full_path, std::vector<std::string>& path_list)
{
    const char* sep = "/\\";
    std::size_t file_sep = full_path.find_first_of(sep);

    if (file_sep > full_path.length())
        return;

    do
    {
        path_list.push_back(full_path.substr(0, file_sep));
        full_path = full_path.substr(file_sep + 1, full_path.length() - 1);
        file_sep = full_path.find_first_of(sep);
    } while (file_sep < full_path.length());

    if (full_path.length() > 0)
    {
        path_list.push_back(full_path);
    }

}

// ----------------------------------------------------------------------------
int32_t make_dir(std::string directory_path, std::string new_folder)
{

    int32_t status = 0;
    directory_path = path_check(directory_path);
        
    bool check = existence_check(directory_path);
    
    if(check)
    {
        std::string full_path = directory_path + new_folder;
#if defined(_WIN32) | defined(__WIN32__) | defined(__WIN32) | defined(_WIN64) | defined(__WIN64)

        std::wstring w_tp = std::wstring(full_path.begin(), full_path.end());
        //status = (int32_t)CreateDirectoryW(w_tp.c_str(), NULL);
        CreateDirectoryW(w_tp.c_str(), NULL);
        status = GetLastError();

#elif defined(__linux__)

        mode_t mode = 0766;
        status = mkdir(full_path.c_str(), mode);
#endif 
    }
    else
    {
        status = ERROR_PATH_NOT_FOUND;
    }

    return status;
    
}	// end of make_dir


// ----------------------------------------------------------------------------
int32_t mkdir(std::string full_path)
{
    int32_t status = 0;
    bool check;
    std::string test_path = "";
    std::vector<std::string> path_list;

    separate_paths(full_path, path_list);

    for (auto s : path_list)
    {
        test_path = test_path + s + "/";
        check = existence_check(test_path);

        if (check == false)
        {
#if defined(_WIN32) | defined(__WIN32__) | defined(__WIN32) | defined(_WIN64) | defined(__WIN64)

            std::wstring w_tp = std::wstring(test_path.begin(), test_path.end());
            //status = (int32_t)CreateDirectoryW(w_tp.c_str(), NULL);
            bool res = CreateDirectoryW(w_tp.c_str(), NULL);

            if(!res)
                status = GetLastError();
            
#elif defined(__linux__)

            mode_t mode = 0766;
            status = mkdir(test_path.c_str(), mode);
#endif            
               
        }
    }

    return status;
}

// ----------------------------------------------------------------------------

#endif	// FILE_OPERATIONS_H_
