#define _CRT_SECURE_NO_WARNINGS

#ifndef GET_CURRENT_TIME_H_
#define GET_CURRENT_TIME_H_

// #include <vector> 
// #include <iostream>
// #include <sstream>
// #include <fstream>
#include <ctime>
#include <string>


// ----------------------------------------------------------------------------------------
time_t convert_time(std::string stime)
{
    // format: HHMMSS
    struct tm timeinfo = { 0 };
    timeinfo.tm_hour = std::stoi(stime.substr(0, 2));
    timeinfo.tm_min = std::stoi(stime.substr(2, 2));
    timeinfo.tm_sec = std::stoi(stime.substr(4, 2));

    return mktime(&timeinfo);
}

// ----------------------------------------------------------------------------------------
time_t convert_date(std::string sdate)
{
    // format: YYYYMMDD
    struct tm timeinfo = { 0 };
    timeinfo.tm_year = std::stoi(sdate.substr(0, 4)) - 1900;
    timeinfo.tm_mon = std::stoi(sdate.substr(4, 2)) - 1;
    timeinfo.tm_mday = std::stoi(sdate.substr(6, 2));

    return mktime(&timeinfo);
}

// ----------------------------------------------------------------------------------------
std::string get_date(time_t now, const std::string &format)
{
    //time_t rawtime;
    struct tm *timeinfo; 
    
    char c_date[32];

    //time(&rawtime);
    timeinfo = localtime(&now);

    strftime(c_date, 9, format.c_str(), timeinfo);

    return (std::string)(c_date);

}   // end of get_date

// ----------------------------------------------------------------------------------------
std::string get_time(time_t now, const std::string &format)
{
    //time_t rawtime;
    struct tm *timeinfo;

    char c_time[32];

    //time(&rawtime);
    timeinfo = localtime(&now);

    strftime(c_time, 7, format.c_str(), timeinfo);

    return (std::string)(c_time);

}   // end of get_time

// ----------------------------------------------------------------------------------------
void get_current_time(std::string &sdate, std::string &stime)
{
	time_t now = time(NULL);
	//struct tm * timeinfo;

	//char c_date[9];
	//char c_time[7];
	//
	//time(&rawtime);
	//timeinfo = localtime(&rawtime);

	//strftime(c_date, 9, "%Y%m%d", timeinfo);
	//strftime(c_time, 7, "%H%M%S", timeinfo);
	//
	//sdate = (std::string)(c_date);
	//stime = (std::string)(c_time);

    sdate = get_date(now, "%Y%m%d");
    stime = get_time(now, "%H%M%S");
	
}	// end of get_current_time

#endif  // GET_CURRENT_TIME_H_